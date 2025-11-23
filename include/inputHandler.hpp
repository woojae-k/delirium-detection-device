#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include <Arduino.h>
#include "config.hpp"
#include "data.hpp"
class InputHandler {
  public:
    void init();
    bool checkJoystick();
    bool checkSelectButton();
    bool checkResetButton();

    JoystickState getJoystickState() { return currentJoyState; };
    int getResetState() {return currentResetState;};
    int getSelectState() {return currentSelectState;};
    void clearButtonStates();

  private:

    // Button Debounce Variables 
    unsigned long lastSelectPressTime = 0;
    unsigned long lastResetPressTime = 0;
    unsigned long debounceDelay = 250; // 250ms delay
    int lastSelectState = HIGH;    
    int lastResetState = HIGH;

    int currentResetState;
    int currentSelectState;

    JoystickState currentJoyState;
    JoystickState lastJoyState;
};

#endif
