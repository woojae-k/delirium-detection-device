/*
 * ESP32 - CAM Quiz Prototype (TFT_eSPI Version)
 *
 * Pins:
 * - Pin 21: Select/Confirm Choice Button
 * - Pin 19: Reset Device Button
 * - Pin 33: Joystick X-Axis (ADC)
 * - Pin 35: Joystick Y-Axis (ADC, input only)
 */

#include <Arduino.h>

#include "displayManager.hpp"
#include "inputHandler.hpp"
#include "questionManager.hpp"

namespace {

QuestionManager questionManager;
InputHandler inputHandler;
DisplayManager displayManager;

class QuizController {
  public:
    void begin() {
      Serial.begin(115200);
      Serial.println("CAM Assessment Device Booting (Standard Fonts)...");

      inputHandler_.init();
      displayManager_.init();
      displayManager_.displayStartScreen();
      awaitingResponse_ = false;
      hasStarted_ = false;
    }

    void loop() {
      if (!hasStarted_) {
        if (inputHandler_.checkSelectButton()) {
          startAssessment();
        } else if (inputHandler_.checkResetButton()) {
          displayManager_.displayStartScreen(); // keep showing start screen
        }
        delay(10);
        return;
      }

      if (inputHandler_.checkResetButton()) {
        resetQuiz();
        return;
      }

      if (quizComplete_) {
        delay(50);
        return;
      }

      // Timeout detection for inactivity on current question
      if (awaitingResponse_) {
        const unsigned long now = millis();
        if (now - questionStartTimeMs_ >= inactivityTimeoutMs_) {
          handleTimeout(now);
          delay(10);
          return;
        }
      }

      if (inputHandler_.checkJoystick()) {
        updateSelection(inputHandler_.getJoystickState());
      }

      if (inputHandler_.checkSelectButton()) {
        handleSelection();
      }

      delay(10);
    }

  private:
    void startAssessment() {
      hasStarted_ = true;
      resetQuiz();
    }

    void resetQuiz() {
      Serial.println("Resetting assessment...");
      questionManager_.reset();
      quizComplete_ = false;
      selectedChoice_ = 0;
      awaitingResponse_ = true;
      displayCurrentQuestion();
    }

    void displayCurrentQuestion() {
      if (questionManager_.isQuizComplete()) {
        awaitingResponse_ = false;
        return;
      }

      displayManager_.showQuestion(questionManager_.getCurrentQuestion(), selectedChoice_);
      questionStartTimeMs_ = millis();
      awaitingResponse_ = true;
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

      logResult(current, chosenAnswer, isCorrect, latency, false);

      questionManager_.nextQuestion();

      if (questionManager_.isQuizComplete()) {
        quizComplete_ = true;
        displayManager_.displayCompletionScreen();
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
        quizComplete_ = true;
        displayManager_.displayCompletionScreen();
        return;
      }

      selectedChoice_ = 0;
      displayCurrentQuestion();
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

    QuestionManager& questionManager_{questionManager};
    InputHandler& inputHandler_{inputHandler};
    DisplayManager& displayManager_{displayManager};

    int selectedChoice_ = 0;
    bool quizComplete_ = false;
    bool hasStarted_ = false;
    bool awaitingResponse_ = true;
    unsigned long questionStartTimeMs_ = 0;
    const unsigned long inactivityTimeoutMs_ = 30000; // 30s timeout
    const unsigned long longReactionThresholdMs_ = 7000; // flag slow responses
};

QuizController controller;

}  // namespace

void setup() {
  controller.begin();
}

void loop() {
  controller.loop();
}
