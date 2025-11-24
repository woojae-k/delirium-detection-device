#ifndef ASSESSMENT_SESSION_HPP
#define ASSESSMENT_SESSION_HPP

#include <Arduino.h>

#include "displayManager.hpp"
#include "inputHandler.hpp"
#include "questionManager.hpp"
#include "storageManager.hpp"

class AssessmentSession {
  public:
    AssessmentSession(QuestionManager& questions, InputHandler& input, DisplayManager& display);

    void start();
    // Returns true when the assessment is complete and the user has pressed SELECT to exit.
    bool loop();
    void reset();

  private:
    void displayCurrentQuestion();
    void updateSelection(JoystickState joystickState);
    void handleSelection();
    void handleTimeout(unsigned long nowMs);
    void finishQuiz();
    void recordAssessment();
    String generatePatientCode();
    void logResult(const Question& q, const char* chosenAnswer, bool isCorrect,
                   unsigned long latencyMs, bool timedOut);

    QuestionManager& questionManager_;
    InputHandler& inputHandler_;
    DisplayManager& displayManager_;

    int selectedChoice_{0};
    bool quizComplete_{false};
    bool awaitingResponse_{false};
    bool loggedAssessment_{false};
    unsigned long questionStartTimeMs_{0};
    const unsigned long inactivityTimeoutMs_ = 10000;
    const unsigned long longReactionThresholdMs_ = 7000;
    int score_{0};
    int patientCounter_{1};
    String currentPatientCode_{"PT-0000"};
};

#endif
