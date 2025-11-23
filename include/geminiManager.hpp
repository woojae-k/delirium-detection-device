#ifndef GEMINI_MANAGER_HPP
#define GEMINI_MANAGER_HPP

#include <Arduino.h>
#include <vector>
#include "data.hpp"

class GeminiManager {
  public:
    bool fetchQuestions(std::vector<Question>& questions);

  private:
    String sendRequest();
    bool parseResponse(const String& jsonResponse, std::vector<Question>& questions);
    String truncateToFit(const String& text, size_t maxLen);
    void sanitizeQuestions(std::vector<Question>& questions);
};

#endif
