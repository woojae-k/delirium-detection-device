#ifndef QUESTION_GENERATOR_HPP
#define QUESTION_GENERATOR_HPP

#include "displayManager.hpp"
#include "geminiManager.hpp"
#include "inputHandler.hpp"
#include "questionManager.hpp"

class QuestionGenerator {
  public:
    QuestionGenerator(DisplayManager& display, GeminiManager& gemini, QuestionManager& questions);

    bool run(InputHandler& input);

  private:
    bool connectToWiFi();
    void disconnectWiFi();
    bool fetchAndUpdateQuestions();

    DisplayManager& displayManager_;
    GeminiManager& geminiManager_;
    QuestionManager& questionManager_;
};

#endif
