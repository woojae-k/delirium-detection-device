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
    String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + String(GEMINI_API_KEY);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // JSON payload
    JsonDocument doc;
    JsonObject content = doc["contents"].add();
    JsonObject part = content["parts"].add();
    part["text"] = "Generate 5 multiple choice questions for delirium detection based on CAM principles: Acute onset, Inattention, Disorganized thinking, Altered level of consciousness. Return ONLY a raw JSON array (no markdown) where each object has: 'prompt' (string), 'choices' (array of 4 strings), 'correctChoice' (int 0-3).";

    String requestBody;
    serializeJson(doc, requestBody);

    Serial.println("Sending request to Gemini...");
    int httpResponseCode = http.POST(requestBody);

    String response = "";
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        response = http.getString();
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
        Serial.println("No text in response");
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
        Question q;
        q.prompt = qObj["prompt"].as<String>();
        JsonArray choices = qObj["choices"];
        for (int i = 0; i < 4; i++) {
            q.choices[i] = choices[i].as<String>();
        }
        q.correctChoice = qObj["correctChoice"].as<int>();
        newQuestions.push_back(q);
    }
    
    questions = newQuestions;
    return true;
}
