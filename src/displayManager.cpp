#include "displayManager.hpp"

void DisplayManager::init() {
  tft_.begin();
  tft_.setRotation(3); // 1 or 3 for landscape (320x240)
  tft_.fillScreen(TFT_BLACK);
  Serial.println("Display initialized.");
}

void DisplayManager::displayStartScreen() {
  tft_.fillScreen(TFT_BLACK);
  tft_.setTextColor(TFT_WHITE, TFT_BLACK);
  tft_.setTextDatum(MC_DATUM);

  // Title
  tft_.setTextSize(1);
  tft_.drawString("CAM Screening", tft_.width() / 2, tft_.height() / 4, 4);

  // Instructions (smaller font to fit)
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextSize(1);
  tft_.drawString("Press SELECT to start", tft_.width() / 2, tft_.height() / 2, 2);
  tft_.drawString("Use joystick to choose", tft_.width() / 2, tft_.height() * 5 / 8, 2);
}

void DisplayManager::displayMenu(int selectedIndex) {
  tft_.fillScreen(TFT_BLACK);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextSize(1);

  const char* options[2] = {"1. Start Test", "2. Export Data"};
  for (int i = 0; i < 2; ++i) {
    const bool isSelected = (i == selectedIndex);
    const uint16_t bg = isSelected ? TFT_WHITE : TFT_BLACK;
    const uint16_t fg = isSelected ? TFT_BLACK : TFT_WHITE;
    const int y = tft_.height() / 2 + (i * 30) - 20;
    tft_.fillRect(20, y - 12, tft_.width() - 40, 28, bg);
    tft_.setTextColor(fg, bg);
    tft_.drawString(options[i], tft_.width() / 2, y, 4);
  }

  tft_.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft_.drawString("CAM Device Menu", tft_.width() / 2, 40, 2);
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

  const int screen_width = tft_.width();
  const int screen_height = tft_.height();
  const int question_area_height = screen_height / 3; // top band for the prompt
  const int option_width = screen_width / 2;
  const int option_height = (screen_height - question_area_height) / 2;

  tft_.fillScreen(TFT_BLACK);
  tft_.setTextColor(TFT_WHITE, TFT_BLACK);

  // Draw wrapped question prompt near the top using a small font for long strings
  tft_.setTextSize(1);
  tft_.setTextDatum(TL_DATUM);
  const int prompt_font = 4;
  const int line_height = tft_.fontHeight(prompt_font) + 2;
  const int max_width = screen_width - 6;

  auto drawWrappedText = [&](const char* text, int startX, int startY) {
    String remaining(text);
    int y = startY;
    while (remaining.length() > 0) {
      String line = remaining;
      while (tft_.textWidth(line, prompt_font) > max_width) {
        int lastSpace = line.lastIndexOf(' ');
        if (lastSpace <= 0) break;
        line.remove(lastSpace);
      }
      if (line.length() == 0) {
        break;
      }
      tft_.drawString(line, startX, y, prompt_font);
      remaining.remove(0, line.length());
      remaining.trim();
      y += line_height;
      if (y > question_area_height - line_height) break; // prevent overflow into answer grid
    }
  };

  drawWrappedText(q_.prompt, 3, 3);

  // Draw the 2x2 grid for the answers
  tft_.drawFastHLine(0, question_area_height, screen_width, TFT_WHITE);
  tft_.drawFastHLine(0, question_area_height + option_height, screen_width, TFT_WHITE);
  tft_.drawFastVLine(option_width, question_area_height, screen_height - question_area_height, TFT_WHITE);
}

void DisplayManager::displayCompletionScreen(const String& patientCode) {
  tft_.fillScreen(TFT_BLACK);
  tft_.setTextColor(TFT_GREEN, TFT_BLACK);
  tft_.setTextSize(1);
  tft_.setTextDatum(MC_DATUM);

  // Split into two lines so it stays within the 320x240 layout
  const int cx = tft_.width() / 2;
  const int cy = tft_.height() / 2;
  tft_.drawString("Assessment", cx, cy - 24, 4);
  tft_.drawString("Complete",   cx, cy + 2, 4);
  tft_.setTextColor(TFT_WHITE, TFT_BLACK);
  tft_.drawString("Patient:", cx, cy + 34, 2);
  tft_.drawString(patientCode, cx, cy + 52, 4);
  tft_.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft_.drawString("Press SELECT to return", cx, cy + 86, 2);

  Serial.println("--- Assessment Complete ---");
  Serial.println("Press RESET (Pin 19) to start over.");
  
}

void DisplayManager::displayExportInstructions() {
  tft_.fillScreen(TFT_BLACK);
  tft_.setTextColor(TFT_WHITE, TFT_BLACK);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextSize(1);

  tft_.drawString("Connect to WiFi:", tft_.width() / 2, 60, 2);
  tft_.drawString("CAM_Device", tft_.width() / 2, 80, 4);
  tft_.drawString("Open browser to", tft_.width() / 2, 130, 2);
  tft_.drawString("192.168.4.1", tft_.width() / 2, 155, 4);
  tft_.drawString("Press SELECT to exit", tft_.width() / 2, 200, 2);
}

void DisplayManager::highlightChoice(int selectedChoice) {
  if (selectedChoice < 0) {
    selectedChoice_ = 0;
  } else if (selectedChoice > 3) {
    selectedChoice_ = 3;
  } else {
    selectedChoice_ = selectedChoice;
  }
  const int screen_width = tft_.width();
  const int screen_height = tft_.height();
  const int question_area_height = screen_height / 3;
  const int option_width = screen_width / 2;
  const int option_height = (screen_height - question_area_height) / 2;

  // Set font properties for choices
  tft_.setTextSize(1);
  tft_.setTextDatum(MC_DATUM);
  const int choice_font_primary = 4;
  const int choice_font_fallback = 2;

  for (int i = 0; i < 4; ++i) {
    const bool isSelected = (i == selectedChoice_);
    const uint16_t bgColor = isSelected ? TFT_WHITE : TFT_BLACK;
    const uint16_t textColor = isSelected ? TFT_BLACK : TFT_WHITE;

    const int col = i % 2;
    const int row = i / 2;
    const int rectX = (col * option_width) + 1;
    const int rectY = question_area_height + (row * option_height) + 1;
    const int rectW = option_width - 2;
    const int rectH = option_height - 2;

    tft_.fillRect(rectX, rectY, rectW, rectH, bgColor);
    tft_.setTextColor(textColor, bgColor);

    // Use largest font that fits in the choice box; fall back if too wide
    int fontToUse = choice_font_primary;
    if (tft_.textWidth(q_.choices[i], choice_font_primary) > rectW - 6) {
      fontToUse = choice_font_fallback;
    }
    tft_.drawString(q_.choices[i], rectX + rectW / 2, rectY + rectH / 2, fontToUse);
  }
}
