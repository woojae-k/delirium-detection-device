#ifndef DATA_HPP
#define DATA_HPP

#include <Arduino.h>

struct Question {
  String prompt;
  String choices[4];
  int    correctChoice; // 0-3
};

enum JoystickState { STATE_NEUTRAL, STATE_LEFT, STATE_RIGHT, STATE_UP, STATE_DOWN };
enum AssessmentState {DEFAULT_QUESTIONS, AI_GENERATED_QUESTIONS};


#endif
