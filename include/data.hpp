#ifndef DATA_HPP
#define DATA_HPP

struct Question {
  const char* prompt;
  const char* choiceA;
  const char* choiceB;
  int         correctChoice;
};

enum JoystickState { STATE_NEUTRAL, STATE_LEFT, STATE_RIGHT };


#endif
