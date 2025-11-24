#ifndef DISPLAY_MANAGER_HPP
#define DISPLAY_MANAGER_HPP

#include "config.hpp"
#include <TFT_eSPI.h> // The main TFT_eSPI library

#include "data.hpp"

class DisplayManager {
  public:
    void init();
    void displayStartScreen();
    void displayMenu(int selectedIndex, AssessmentState currentStatus = DEFAULT_QUESTIONS);
    void showQuestion(const Question& question, int selectedChoice = 0);
    void highlightChoice(int selectedChoice = 0);
    void displayCompletionScreen(const String& patientCode);
    void displayExportInstructions();
    void displayStatus(const String& msg);

  private:
    void displayQuestion();
    void setQuestion(Question q);

    // Initialize the TFT display
    TFT_eSPI tft_ = TFT_eSPI(); 
    Question q_;
    int selectedChoice_;
};

#endif
