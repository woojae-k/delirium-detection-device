#ifndef DATA_HPP
#define DATA_HPP

struct Question {
  const char* prompt;
  const char* choices[4];
  int         correctChoice; // 0-3
};

enum JoystickState { STATE_NEUTRAL, STATE_LEFT, STATE_RIGHT, STATE_UP, STATE_DOWN };


#endif
