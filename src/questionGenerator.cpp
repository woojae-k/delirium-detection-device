#include "questionGenerator.hpp"

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

#include "config.hpp"

QuestionGenerator::QuestionGenerator(DisplayManager& display, GeminiManager& gemini,
                                     QuestionManager& questions)
    : displayManager_(display), geminiManager_(gemini), questionManager_(questions) {}

bool QuestionGenerator::run(InputHandler& input) {
  displayManager_.displayStatus("Connecting to WiFi...");
  Serial.println("Connecting to WiFi...");

  bool success = false;
  if (connectToWiFi()) {
    displayManager_.displayStatus("Generating Qs...");
    Serial.println("\nConnected! Fetching questions...");
    success = fetchAndUpdateQuestions();
  } else {
    displayManager_.displayStatus("WiFi Failed");
    Serial.println("\nWiFi Connection Failed.");
    delay(2000);
  }

  disconnectWiFi();
  input.clearButtonStates();
  return success;
}

bool QuestionGenerator::connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  return WiFi.status() == WL_CONNECTED;
}

void QuestionGenerator::disconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);
  }
  WiFi.mode(WIFI_OFF);
}

bool QuestionGenerator::fetchAndUpdateQuestions() {
  std::vector<Question> newQuestions;
  if (geminiManager_.fetchQuestions(newQuestions)) {
    questionManager_.updateQuestions(newQuestions);
    displayManager_.displayStatus("Success!");
    Serial.println("Questions updated!");
    delay(2000);
    return true;
  } else {
    displayManager_.displayStatus("Failed to fetch");
    Serial.println("Failed to fetch questions.");
    delay(2000);
    return false;
  }
}
