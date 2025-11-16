#include "displayManager.hpp"

void DisplayManager::init() {
  tft_.begin();
  tft_.setRotation(1); // 1 or 3 for landscape (320x240)
  tft_.fillScreen(TFT_BLACK);
  Serial.println("Display initialized.");
}

void DisplayManager::showQuestion(const Question& question, int selectedChoice) {
  setQuestion(question);
  displayQuestion();
  highlightChoice(selectedChoice);
}

void DisplayManager::setQuestion(Question q) {
  q_ = q;
}

void DisplayManager::displayQuestion() {

  int half_width = tft_.width() / 2;
  int half_height = tft_.height() / 2;

  tft_.fillScreen(TFT_BLACK);
  tft_.setTextColor(TFT_WHITE, TFT_BLACK);

  // --- Draw Question Prompt (Font 2, Size 2) ---
  tft_.setTextSize(2); // Size 2 multiplier
  tft_.setTextDatum(TC_DATUM);
  // Use Font 2 (16px) * Size 2 = 32px high
  tft_.drawString(q_.prompt, half_width, 20, 2); 

  // --- Draw Layout Lines ---
  tft_.drawFastHLine(0, half_height, tft_.width(), TFT_WHITE);
  tft_.drawFastVLine(half_width, half_height, half_height, TFT_WHITE);

  // --- Draw Choices (Font 4, Size 2) ---
  tft_.setTextSize(2); // Size 2 multiplier
  tft_.setTextDatum(MC_DATUM);
  // Use Font 4 (26px) * Size 2 = 52px high
  
  // Choice A (Left) - Un-highlighted
  tft_.drawString(q_.choiceA, tft_.width() / 4, tft_.height() * 3 / 4, 4);
  // Choice B (Right) - Un-highlighted
  tft_.drawString(q_.choiceB, tft_.width() * 3 / 4, tft_.height() * 3 / 4, 4);
}

void DisplayManager::displayCompletionScreen() {
    tft_.fillScreen(TFT_BLACK);
  tft_.setTextColor(TFT_GREEN, TFT_BLACK);
  
  // Use Font 4, Size 2
  tft_.setTextSize(2);
  tft_.setTextDatum(MC_DATUM);
  tft_.drawString("Assessment Complete", tft_.width() / 2, tft_.height() / 2, 4);

  Serial.println("--- Assessment Complete ---");
  Serial.println("Press RESET (Pin 22) to start over.");
  
}

void DisplayManager::highlightChoice(int selectedChoice) {
  selectedChoice_ = selectedChoice;
  int half_width = tft_.width() / 2;
  int half_height = tft_.height() / 2;
  int left_box_x = tft_.width() / 4;
  int right_box_x = tft_.width() * 3 / 4;
  int box_y = tft_.height() * 3 / 4;
  
  // Set font properties for choices
  tft_.setTextSize(2);
  tft_.setTextDatum(MC_DATUM);

  if (selectedChoice_ == 0) {
    // --- HIGHLIGHT LEFT ---
    tft_.fillRect(0, half_height + 1, half_width - 1, half_height - 1, TFT_WHITE);
    tft_.setTextColor(TFT_BLACK, TFT_WHITE);
    tft_.drawString(q_.choiceA, left_box_x, box_y, 4);

    // --- DE-HIGHLIGHT RIGHT ---
    tft_.fillRect(half_width + 1, half_height + 1, half_width - 1, half_height - 1, TFT_BLACK);
    tft_.setTextColor(TFT_WHITE, TFT_BLACK);
    tft_.drawString(q_.choiceB, right_box_x, box_y, 4);
  } else {
    // --- DE-HIGHLIGHT LEFT ---
    tft_.fillRect(0, half_height + 1, half_width - 1, half_height - 1, TFT_BLACK);
    tft_.setTextColor(TFT_WHITE, TFT_BLACK);
    tft_.drawString(q_.choiceA, left_box_x, box_y, 4);

    // --- HIGHLIGHT RIGHT ---
    tft_.fillRect(half_width + 1, half_height + 1, half_width - 1, half_height - 1, TFT_WHITE);
    tft_.setTextColor(TFT_BLACK, TFT_WHITE);
    tft_.drawString(q_.choiceB, right_box_x, box_y, 4);
  }
}
