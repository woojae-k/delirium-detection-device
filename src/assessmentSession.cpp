#include "assessmentSession.hpp"

#include <Arduino.h>

AssessmentSession::AssessmentSession(QuestionManager& questions, InputHandler& input,
                                     DisplayManager& display)
    : questionManager_(questions), inputHandler_(input), displayManager_(display) {}

void AssessmentSession::start() {
  Serial.println("Starting assessment...");
  questionManager_.reset();
  selectedChoice_ = 0;
  quizComplete_ = false;
  loggedAssessment_ = false;
  awaitingResponse_ = false;
  score_ = 0;
  currentPatientCode_ = generatePatientCode();
  inputHandler_.clearButtonStates();
  displayCurrentQuestion();
}

bool AssessmentSession::loop() {
  if (quizComplete_) {
    return inputHandler_.checkSelectButton();
  }

  const unsigned long now = millis();
  if (awaitingResponse_ && (now - questionStartTimeMs_ >= inactivityTimeoutMs_)) {
    handleTimeout(now);
    return false;
  }

  if (inputHandler_.checkJoystick()) {
    updateSelection(inputHandler_.getJoystickState());
  }

  if (inputHandler_.checkSelectButton()) {
    handleSelection();
  }

  return false;
}

void AssessmentSession::reset() {
  awaitingResponse_ = false;
  quizComplete_ = false;
  loggedAssessment_ = false;
  selectedChoice_ = 0;
  score_ = 0;
  questionStartTimeMs_ = 0;
  questionManager_.reset();
}

void AssessmentSession::displayCurrentQuestion() {
  if (questionManager_.isQuizComplete()) {
    finishQuiz();
    return;
  }

  awaitingResponse_ = true;
  displayManager_.showQuestion(questionManager_.getCurrentQuestion(), selectedChoice_);
  questionStartTimeMs_ = millis();
}

void AssessmentSession::updateSelection(JoystickState joystickState) {
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

void AssessmentSession::handleSelection() {
  if (questionManager_.isQuizComplete()) {
    return;
  }

  awaitingResponse_ = false;
  const unsigned long now = millis();
  const unsigned long latency = now - questionStartTimeMs_;

  const Question current = questionManager_.getCurrentQuestion();
  String chosenAnswer = current.choices[selectedChoice_];
  const bool isCorrect = (selectedChoice_ == current.correctChoice);
  if (isCorrect) {
    score_++;
  }

  logResult(current, chosenAnswer.c_str(), isCorrect, latency, false);

  questionManager_.nextQuestion();

  if (questionManager_.isQuizComplete()) {
    finishQuiz();
    return;
  }

  selectedChoice_ = 0;
  displayCurrentQuestion();
}

void AssessmentSession::handleTimeout(unsigned long nowMs) {
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

void AssessmentSession::finishQuiz() {
  quizComplete_ = true;
  awaitingResponse_ = false;
  displayManager_.displayCompletionScreen(currentPatientCode_);
  if (!loggedAssessment_) {
    recordAssessment();
    loggedAssessment_ = true;
  }
}

void AssessmentSession::recordAssessment() {
  const int totalQuestions = questionManager_.getQuestionCount();
  const String risk = (score_ >= (totalQuestions / 2)) ? "Low" : "Elevated";

  if (Storage::appendResultToCSV(score_, risk, currentPatientCode_)) {
    Serial.println("Result saved to /data.csv");
  } else {
    Serial.println("Failed to save result to /data.csv");
  }
}

String AssessmentSession::generatePatientCode() {
  char buffer[12];
  snprintf(buffer, sizeof(buffer), "PT-%04d", patientCounter_++);
  return String(buffer);
}

void AssessmentSession::logResult(const Question& q, const char* chosenAnswer, bool isCorrect,
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
