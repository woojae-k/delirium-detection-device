#ifndef DISPLAY_MANAGER_HPP
#define DISPLAY_MANAGER_HPP

#include "config.hpp"
#include <TFT_eSPI.h> // The main TFT_eSPI library

#include "data.hpp"

class DisplayManager {
  public:
    void init();
    void showQuestion(const Question& question, int selectedChoice = 0);
    void highlightChoice(int selectedChoice = 0);
    void displayCompletionScreen();

  private:
    void displayQuestion();
    void setQuestion(Question q);

    TFT_eSPI tft_ = TFT_eSPI(); // Initialize the TFT display
    Question q_;
    int selectedChoice_;
};

#endif
