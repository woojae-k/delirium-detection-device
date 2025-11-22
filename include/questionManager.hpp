#ifndef QUESTION_MANAGER_HPP
#define QUESTION_MANAGER_HPP

#include <SPI.h>
#include "data.hpp"

class QuestionManager {
  public:
    QuestionManager();
    void nextQuestion();
    void reset();
    const Question& getCurrentQuestion();
    int getQuestionCount() const;
    int getCurrentQuestionIndex() const;
    bool isQuizComplete() const;

  private:
    static const Question baseQuestions_[];
    static const int questionCount_;

    Question workingQuestion_;
    int questionOrder_[10]; // max supported questions
    int currentQuestionIndex_{0};

    void shuffleQuestions();
    void shuffleChoices(Question& q);
    void buildWorkingQuestion(int baseIndex);
};


#endif
