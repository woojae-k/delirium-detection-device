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
    int getQuestionCount();
    int getCurrentQuestionIndex();
    bool isQuizComplete();

  private:
    static const Question questions[];
    int currentQuestionIndex_{0};
};


#endif
