#include "questionManager.hpp"
#include <Arduino.h>
#include <esp_random.h>
#include "config.hpp"

QuestionManager::QuestionManager() : currentQuestionIndex_(0) {
  // Initialize with default questions
  defaultQuestions_ = {
      { "Which season has snow?", { "Summer", "Winter", "Spring", "Fall" }, 1 },
      { "What kind of place is this?", { "Hotel", "Hospital or Home", "Train station", "Bank" }, 1 },
      { "When do you eat breakfast?", { "Morning", "Night", "Midnight", "Never" }, 0 },
      { "What is 'FISH' spelled backwards?", { "SHIF", "HSIF", "FIHS", "IHFS" }, 1 },
      { "Which number is the largest?", { "89", "98", "102", "76" }, 2 },
      { "What is 20 minus 3?", { "15", "16", "17", "23" }, 2 },
      { "How many 'A's in S-A-V-E-A-H-A?", { "1", "2", "3", "4" }, 2 },
      { "Will a stone float on water?", { "Yes, it floats", "No, it sinks", "It melts", "It flies" }, 1 },
      { "Where do fish live?", { "In trees", "In the water", "In the dirt", "In the sky" }, 1 },
      { "What is a hammer used for?", { "Hitting nails", "Cutting wood", "Cutting paper", "Eating food" }, 0 }
  };
  questions_ = defaultQuestions_;
  reset();
}

void QuestionManager::reset() {
  shuffleQuestions();
  currentQuestionIndex_ = 0;
  if (!questions_.empty()) {
    buildWorkingQuestion(questionOrder_[0]);
  }
}

void QuestionManager::nextQuestion() {
  if (currentQuestionIndex_ + 1 < getQuestionCount()) {
    currentQuestionIndex_++;
    buildWorkingQuestion(questionOrder_[currentQuestionIndex_]);
  } else {
    currentQuestionIndex_ = getQuestionCount();
  }
}

const Question& QuestionManager::getCurrentQuestion() { return workingQuestion_; }

int QuestionManager::getQuestionCount() const { return questions_.size(); }

int QuestionManager::getCurrentQuestionIndex() const { return currentQuestionIndex_; }

bool QuestionManager::isQuizComplete() const { return currentQuestionIndex_ >= getQuestionCount(); }

void QuestionManager::updateQuestions(const std::vector<Question>& newQuestions) {
    questions_ = newQuestions;
    reset();
}

void QuestionManager::restoreDefaultQuestions() {
    questions_ = defaultQuestions_;
    reset();
}

void QuestionManager::shuffleQuestions() {
  int count = questions_.size();
  questionOrder_.resize(count);
  for (int i = 0; i < count; ++i) {
    questionOrder_[i] = i;
  }

  randomSeed(esp_random());
  for (int i = count - 1; i > 0; --i) {
    int j = random(i + 1);
    int temp = questionOrder_[i];
    questionOrder_[i] = questionOrder_[j];
    questionOrder_[j] = temp;
  }
}

void QuestionManager::shuffleChoices(Question& q) {
  int order[4] = {0, 1, 2, 3};
  for (int i = 3; i > 0; --i) {
    int j = random(i + 1);
    int tmp = order[i];
    order[i] = order[j];
    order[j] = tmp;
  }

  String shuffled[4];
  int newCorrect = 0;
  for (int i = 0; i < 4; ++i) {
    shuffled[i] = q.choices[order[i]];
    if (order[i] == q.correctChoice) {
      newCorrect = i;
    }
  }

  for (int i = 0; i < 4; ++i) {
    q.choices[i] = shuffled[i];
  }
  q.correctChoice = newCorrect;
}

void QuestionManager::buildWorkingQuestion(int baseIndex) {
  workingQuestion_.prompt = questions_[baseIndex].prompt;
  for (int i = 0; i < 4; ++i) {
    workingQuestion_.choices[i] = questions_[baseIndex].choices[i];
  }
  workingQuestion_.correctChoice = questions_[baseIndex].correctChoice;
  shuffleChoices(workingQuestion_);
}
