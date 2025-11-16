#include "questionManager.hpp"

const Question QuestionManager::questions[] = {
  { "Which number is bigger?", "9.12", "9.9", 1 },
  { "What day is it today?", "Monday", "Tuesday", 0 },
  { "Spell 'WORLD'", "W-O-R-L-D", "W-O-L-R-D", 0 },
  { "Is this a flower?", "YES", "NO", 1 }
};

QuestionManager::QuestionManager() : currentQuestionIndex_(0) {}

void QuestionManager::reset() {
  currentQuestionIndex_ = 0;
}

void QuestionManager::nextQuestion() {
  if (currentQuestionIndex_ + 1 < getQuestionCount()) {
    currentQuestionIndex_++;
  } else {
    currentQuestionIndex_ = getQuestionCount();
  }
}

const Question& QuestionManager::getCurrentQuestion() {
  return questions[currentQuestionIndex_];
}

int QuestionManager::getQuestionCount() {
  return sizeof(questions) / sizeof(questions[0]);
}

int QuestionManager::getCurrentQuestionIndex() {
  return currentQuestionIndex_;
}

bool QuestionManager::isQuizComplete() {
  return currentQuestionIndex_ >= getQuestionCount();
}
