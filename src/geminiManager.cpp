#include "geminiManager.hpp"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.hpp"

bool GeminiManager::fetchQuestions(std::vector<Question>& questions) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return false;
    }

    String jsonResponse = sendRequest();
    if (jsonResponse.isEmpty()) {
        return false;
    }

    return parseResponse(jsonResponse, questions);
}

String GeminiManager::sendRequest() {
    HTTPClient http;
    String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent";
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-goog-api-key", GEMINI_API_KEY);

    // JSON payload
    // Keep questions short to fit large-font display; return exactly 10 CAM-aligned items
    String prompt = "Generate simple, elderly-friendly, joystick-navigable four-choice multiple-choice questions for a handheld delirium-screening device with no microphone and only a screen and joystick, ensuring each question is a purely visual, task-based prompt that objectively tests the four CAM criteria—acute onset through orientation tasks, inattention through short recall or visual tracking tasks, disorganized thinking through logic or pattern-continuation tasks, and altered level of consciousness through simple reaction- or attention-based visual tasks—while keeping every question extremely clear, small-screen compatible, and formatted only as a question followed by four answer options (A, B, C, D). Keep each prompt under 70 characters and each choice under 18 characters. Return ONLY a raw JSON array of 10 objects, each with: 'prompt' (string), 'choices' (array of 4 strings), 'correctChoice' (int 0-3). No numbering, no markdown, no explanations.";
    String requestBody = "{\"contents\":[{\"parts\":[{\"text\":\"" + prompt + "\"}]}]}";
    
    Serial.println("Request Body: " + requestBody);

    Serial.println("Sending request to Gemini...");
    int httpResponseCode = http.POST(requestBody);

    String response = "";
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        response = http.getString();
        Serial.println("API Response: " + response);
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return response;
}

bool GeminiManager::parseResponse(const String& jsonResponse, std::vector<Question>& questions) {
    // The response from Gemini is nested.
    // { "candidates": [ { "content": { "parts": [ { "text": "JSON_STRING" } ] } } ] }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }

    const char* text = doc["candidates"][0]["content"]["parts"][0]["text"];
    if (!text) {
        Serial.println("No text in response. Full JSON:");
        serializeJson(doc, Serial);
        Serial.println();
        return false;
    }

    // Clean up markdown code blocks if present
    String cleanText = String(text);
    cleanText.replace("```json", "");
    cleanText.replace("```", "");
    cleanText.trim();

    JsonDocument questionsDoc;
    error = deserializeJson(questionsDoc, cleanText);

    if (error) {
        Serial.print("deserializeJson() for questions failed: ");
        Serial.println(error.c_str());
        Serial.println("Raw text: " + cleanText);
        return false;
    }

    JsonArray arr = questionsDoc.as<JsonArray>();
    if (arr.isNull()) {
        Serial.println("Parsed JSON is not an array");
        return false;
    }

    std::vector<Question> newQuestions;
    for (JsonObject qObj : arr) {
        if (!qObj["prompt"].is<String>() || !qObj["choices"].is<JsonArray>()) {
            continue;
        }

        JsonArray choices = qObj["choices"];
        if (choices.size() < 4) {
            continue;
        }

        Question q;
        q.prompt = qObj["prompt"].as<String>();
        for (int i = 0; i < 4; i++) {
            q.choices[i] = choices[i].as<String>();
        }
        q.correctChoice = qObj["correctChoice"].as<int>();
        newQuestions.push_back(q);
    }
    
    sanitizeQuestions(newQuestions);
    if (newQuestions.size() < 10) {
        Serial.println("Not enough valid questions returned from Gemini.");
        return false;
    }

    // Ensure exactly 10 questions for consistent scoring and display
    if (newQuestions.size() > 10) {
        newQuestions.resize(10);
    }

    questions = newQuestions;
    return true;
}

String GeminiManager::truncateToFit(const String& text, size_t maxLen) {
    if (text.length() <= maxLen) return text;
    if (maxLen <= 3) return text.substring(0, maxLen);
    return text.substring(0, maxLen - 3) + "...";
}

void GeminiManager::sanitizeQuestions(std::vector<Question>& questions) {
    const size_t maxPromptLen = 80;  // Fits within 3 lines of font 4 at 320px width
    const size_t maxChoiceLen = 22;  // Fits large-font choices inside 2x2 grid

    for (auto& q : questions) {
        q.prompt.trim();
        q.prompt = truncateToFit(q.prompt, maxPromptLen);
        for (int i = 0; i < 4; ++i) {
            q.choices[i].trim();
            q.choices[i] = truncateToFit(q.choices[i], maxChoiceLen);
        }
        // Keep correctChoice within bounds
        if (q.correctChoice < 0 || q.correctChoice > 3) {
            q.correctChoice = 0;
        }
    }
}
