#include <Arduino.h>
#include <WiFi.h>

#include "displayManager.hpp"
#include "inputHandler.hpp"
#include "questionManager.hpp"
#include "storageManager.hpp"
#include "webServerManager.hpp"

enum AppState { STATE_MENU, STATE_TEST, STATE_EXPORT };

class AppController {
  public:
    void begin() {
      Serial.begin(115200);
      Serial.println("CAM Assessment Device Booting...");

      Storage::begin();
      inputHandler_.init();
      displayManager_.init();
      enterMenu();
    }

    void loop() {
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
      }
      delay(10);
    }

  private:
    void enterMenu() {
      state_ = STATE_MENU;
      menuIndex_ = 0;
      displayManager_.displayMenu(menuIndex_);
    }

    void handleMenu() {
      if (inputHandler_.checkJoystick()) {
        const JoystickState js = inputHandler_.getJoystickState();
        if (js == STATE_UP) {
          menuIndex_ = 0;
        } else if (js == STATE_DOWN) {
          menuIndex_ = 1;
        }
        displayManager_.displayMenu(menuIndex_);
      }

      if (inputHandler_.checkSelectButton()) {
        if (menuIndex_ == 0) {
          startTest();
        } else {
          enterExport();
        }
      }
    }

    void startTest() {
      Serial.println("Starting assessment...");
      state_ = STATE_TEST;
      questionManager_.reset();
      selectedChoice_ = 0;
      quizComplete_ = false;
      loggedAssessment_ = false;
      score_ = 0;
      currentPatientCode_ = generatePatientCode();
      displayCurrentQuestion();
    }

    void handleTest() {
      if (inputHandler_.checkResetButton()) {
        enterMenu();
        return;
      }

      if (quizComplete_) {
        if (inputHandler_.checkSelectButton()) {
          enterMenu();
        }
        return;
      }

      const unsigned long now = millis();
      if (awaitingResponse_ && (now - questionStartTimeMs_ >= inactivityTimeoutMs_)) {
        handleTimeout(now);
        return;
      }

      if (inputHandler_.checkJoystick()) {
        updateSelection(inputHandler_.getJoystickState());
      }

      if (inputHandler_.checkSelectButton()) {
        handleSelection();
      }
    }

    void enterExport() {
      state_ = STATE_EXPORT;
      displayManager_.displayExportInstructions();
      if (!webServerManager_.startExportServer()) {
        Serial.println("Failed to start export server");
      }
    }

    void handleExport() {
      webServerManager_.loop();
      if (inputHandler_.checkSelectButton()) {
        webServerManager_.stopExportServer();
        enterMenu();
      }
    }

    void displayCurrentQuestion() {
      if (questionManager_.isQuizComplete()) {
        finishQuiz();
        return;
      }

      awaitingResponse_ = true;
      displayManager_.showQuestion(questionManager_.getCurrentQuestion(), selectedChoice_);
      questionStartTimeMs_ = millis();
    }

    void updateSelection(JoystickState joystickState) {
      int col = selectedChoice_ % 2;
      int row = selectedChoice_ / 2;

      switch (joystickState) {
        case STATE_LEFT:
          col = max(0, col - 1);
          break;
        case STATE_RIGHT:
          col = min(1, col + 1);
          break;
        case STATE_UP:
          row = max(0, row - 1);
          break;
        case STATE_DOWN:
          row = min(1, row + 1);
          break;
        default:
          return;
      }

      const int newSelection = (row * 2) + col;
      if (newSelection != selectedChoice_) {
        selectedChoice_ = newSelection;
        displayManager_.highlightChoice(selectedChoice_);
      }
    }

    void handleSelection() {
      if (questionManager_.isQuizComplete()) {
        return;
      }

      awaitingResponse_ = false;
      const unsigned long now = millis();
      const unsigned long latency = now - questionStartTimeMs_;

      const Question current = questionManager_.getCurrentQuestion();
      const char* chosenAnswer = current.choices[selectedChoice_];
      const bool isCorrect = (selectedChoice_ == current.correctChoice);
      if (isCorrect) {
        score_++;
      }

      logResult(current, chosenAnswer, isCorrect, latency, false);

      questionManager_.nextQuestion();

      if (questionManager_.isQuizComplete()) {
        finishQuiz();
        return;
      }

      selectedChoice_ = 0;
      displayCurrentQuestion();
    }

    void handleTimeout(unsigned long nowMs) {
      awaitingResponse_ = false;
      const unsigned long latency = nowMs - questionStartTimeMs_;
      const Question current = questionManager_.getCurrentQuestion();
      logResult(current, "Unresponsive/Sleeping", false, latency, true);

      questionManager_.nextQuestion();
      if (questionManager_.isQuizComplete()) {
        finishQuiz();
        return;
      }

      selectedChoice_ = 0;
      displayCurrentQuestion();
    }

    void finishQuiz() {
      quizComplete_ = true;
      awaitingResponse_ = false;
      displayManager_.displayCompletionScreen(currentPatientCode_);
      if (!loggedAssessment_) {
        recordAssessment();
        loggedAssessment_ = true;
      }
    }

    void recordAssessment() {
      const int totalQuestions = questionManager_.getQuestionCount();
      const String timestamp = String(millis());
      const String risk = (score_ >= (totalQuestions / 2)) ? "Low" : "Elevated";
      const int additionalCorrect = max(0, score_ - lastScore_);
      lastScore_ = score_;

      if (Storage::appendResultToCSV(timestamp, score_, risk, currentPatientCode_, additionalCorrect)) {
        Serial.println("Result saved to /data.csv");
      } else {
        Serial.println("Failed to save result to /data.csv");
      }
    }

    String generatePatientCode() {
      char buffer[12];
      snprintf(buffer, sizeof(buffer), "PT-%04d", patientCounter_++);
      return String(buffer);
    }

    void logResult(const Question& q, const char* chosenAnswer, bool isCorrect,
                   unsigned long latencyMs, bool timedOut) {
      Serial.print("Q: ");
      Serial.print(q.prompt);
      Serial.print(" | Selected: ");
      Serial.print(chosenAnswer);
      Serial.print(" | Correct: ");
      Serial.print(isCorrect ? "YES" : "NO");
      Serial.print(" | Latency(ms): ");
      Serial.print(latencyMs);
      if (timedOut) {
        Serial.print(" | TIMEOUT: Unresponsive/Sleeping (Feature 4)");
      } else if (latencyMs > longReactionThresholdMs_) {
        Serial.print(" | FLAG: Slow response");
      }
      Serial.println();
    }

    QuestionManager questionManager_;
    InputHandler inputHandler_;
    DisplayManager displayManager_;
    WebServerManager webServerManager_;

    AppState state_{STATE_MENU};
    int menuIndex_{0};

    int selectedChoice_{0};
    bool quizComplete_{false};
    bool awaitingResponse_{false};
    bool loggedAssessment_{false};
    unsigned long questionStartTimeMs_{0};
    const unsigned long inactivityTimeoutMs_ = 30000;
    const unsigned long longReactionThresholdMs_ = 7000;
    int score_{0};
    int lastScore_{0};
    int patientCounter_{1};
    String currentPatientCode_{"PT-0000"};
};

AppController controller;

void setup() { controller.begin(); }

void loop() { controller.loop(); }
