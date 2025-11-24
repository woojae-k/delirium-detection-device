#include "appController.hpp"

#include <Arduino.h>
#include <WiFi.h>

AppController::AppController()
    : assessmentSession_(questionManager_, inputHandler_, displayManager_),
      exportSession_(webServerManager_, displayManager_),
      questionGenerator_(displayManager_, geminiManager_, questionManager_) {}

void AppController::begin() {
  Serial.begin(115200);
  Serial.println("CAM Assessment Device Booting...");

  Storage::begin();
  inputHandler_.init();
  displayManager_.init();
  enterMenu();
}

void AppController::loop() {
  if (inputHandler_.checkResetButton()) {
    resetToMenu();
    return;
  }

  switch (state_) {
    case STATE_MENU:
      handleMenu();
      break;
    case STATE_TEST:
      handleTest();
      break;
    case STATE_EXPORT:
      handleExport();
      break;
    case STATE_GENERATE:
      handleGenerate();
      break;
  }
  delay(10);
}

void AppController::enterMenu() {
  state_ = STATE_MENU;
  menuIndex_ = 0;
  inputHandler_.clearButtonStates();
  displayManager_.displayMenu(menuIndex_, current_status_);
}

void AppController::handleMenu() {
  const int menuMaxIndex = 3; // 4 menu items: Test, Export, Generate AI Qs, Default Qs
  if (inputHandler_.checkJoystick()) {
    const JoystickState js = inputHandler_.getJoystickState();
    if (js == STATE_UP) {
      menuIndex_ = max(0, menuIndex_ - 1);
    } else if (js == STATE_DOWN) {
      menuIndex_ = min(menuMaxIndex, menuIndex_ + 1);
    }
    displayManager_.displayMenu(menuIndex_, current_status_);
  }

  if (inputHandler_.checkSelectButton()) {
    if (menuIndex_ == 0) {
      startTest();
    } else if (menuIndex_ == 1) {
      enterExport();
    } else if (menuIndex_ == 2) {
      enterGenerate();
    } else {
      useDefaultQuestions();
    }
  }
}

void AppController::startTest() {
  state_ = STATE_TEST;
  assessmentSession_.start();
}

void AppController::handleTest() {
  if (assessmentSession_.loop()) {
    assessmentSession_.reset();
    enterMenu();
  }
}

void AppController::enterExport() {
  state_ = STATE_EXPORT;
  exportSession_.start();
}

void AppController::handleExport() {
  if (exportSession_.loop(inputHandler_)) {
    enterMenu();
  }
}

void AppController::enterGenerate() { state_ = STATE_GENERATE; }

void AppController::handleGenerate() {
  const bool generated = questionGenerator_.run(inputHandler_);
  if (generated) {
    current_status_ = AI_GENERATED_QUESTIONS;
  }
  enterMenu();
}

void AppController::resetToMenu() {
  if (state_ == STATE_EXPORT && webServerManager_.isRunning()) {
    exportSession_.stop();
  }
  if (WiFi.status() == WL_CONNECTED || state_ == STATE_GENERATE) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
  assessmentSession_.reset();
  inputHandler_.clearButtonStates();
  enterMenu();
  Serial.println("Reset button pressed: returning to main menu.");
}

void AppController::useDefaultQuestions() {
  questionManager_.restoreDefaultQuestions();
  displayManager_.displayStatus("Default Qs ready");
  delay(1200);
  current_status_ = DEFAULT_QUESTIONS;
  enterMenu();
}
