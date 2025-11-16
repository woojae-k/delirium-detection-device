/*
 * ESP32 - CAM Quiz Prototype (TFT_eSPI Version)
 *
 * Pins:
 * - Pin 21: Select/Confirm Choice Button
 * - Pin 22: Reset Device Button
 * - Pin 33: Joystick X-Axis
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
      resetQuiz();
    }

    void loop() {
      if (inputHandler_.checkResetButton()) {
        resetQuiz();
        return;
      }

      if (quizComplete_) {
        delay(50);
        return;
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
    void resetQuiz() {
      Serial.println("Resetting assessment...");
      questionManager_.reset();
      quizComplete_ = false;
      selectedChoice_ = 0;
      displayCurrentQuestion();
    }

    void displayCurrentQuestion() {
      if (questionManager_.isQuizComplete()) {
        return;
      }

      displayManager_.showQuestion(questionManager_.getCurrentQuestion(), selectedChoice_);
    }

    void updateSelection(JoystickState joystickState) {
      int newSelection = selectedChoice_;
      if (joystickState == STATE_LEFT) {
        newSelection = 0;
      } else if (joystickState == STATE_RIGHT) {
        newSelection = 1;
      } else {
        return;
      }

      if (newSelection == selectedChoice_) {
        return;
      }

      selectedChoice_ = newSelection;
      displayManager_.highlightChoice(selectedChoice_);
    }

    void handleSelection() {
      if (questionManager_.isQuizComplete()) {
        return;
      }

      const Question current = questionManager_.getCurrentQuestion();
      const char* chosenAnswer = (selectedChoice_ == 0) ? current.choiceA : current.choiceB;
      const bool isCorrect = (selectedChoice_ == current.correctChoice);

      Serial.print("Q: ");
      Serial.print(current.prompt);
      Serial.print(" | Selected: ");
      Serial.print(chosenAnswer);
      Serial.print(" | Correct: ");
      Serial.println(isCorrect ? "YES" : "NO");

      questionManager_.nextQuestion();

      if (questionManager_.isQuizComplete()) {
        quizComplete_ = true;
        displayManager_.displayCompletionScreen();
        return;
      }

      selectedChoice_ = 0;
      displayCurrentQuestion();
    }

    QuestionManager& questionManager_{questionManager};
    InputHandler& inputHandler_{inputHandler};
    DisplayManager& displayManager_{displayManager};

    int selectedChoice_ = 0;
    bool quizComplete_ = false;
};

QuizController controller;

}  // namespace

void setup() {
  controller.begin();
}

void loop() {
  controller.loop();
}
