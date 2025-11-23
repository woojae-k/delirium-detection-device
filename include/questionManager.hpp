#ifndef QUESTION_MANAGER_HPP
#define QUESTION_MANAGER_HPP

#include <SPI.h>
#include <vector>
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
    void updateQuestions(const std::vector<Question>& newQuestions);

  private:
    
  private:
    std::vector<Question> questions_;
    
    Question workingQuestion_;
    std::vector<int> questionOrder_;
    int currentQuestionIndex_{0};

    void shuffleQuestions();
    void shuffleChoices(Question& q);
    void buildWorkingQuestion(int baseIndex);
};


#endif
