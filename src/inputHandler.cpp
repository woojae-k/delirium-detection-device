#include "inputHandler.hpp"


void InputHandler::init() {
  pinMode(SELECT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  currentJoyState = STATE_NEUTRAL;
  lastJoyState = STATE_NEUTRAL;
}

bool InputHandler::checkJoystick() {
  int joyX = analogRead(JOYSTICK_X_PIN);
  int joyY = analogRead(JOYSTICK_Y_PIN);
  currentJoyState = STATE_NEUTRAL;

  // Prioritize vertical movement when both axes are deflected outside the deadzone
  if (joyY < JOY_THRESHOLD_LOW) {
    currentJoyState = STATE_UP;
  } else if (joyY > JOY_THRESHOLD_HIGH) {
    currentJoyState = STATE_DOWN;
  } else if (joyX < JOY_THRESHOLD_LOW) {
    currentJoyState = STATE_LEFT;
  } else if (joyX > JOY_THRESHOLD_HIGH) {
    currentJoyState = STATE_RIGHT;
  } else if (joyX > JOY_DEADZONE_LOW && joyX < JOY_DEADZONE_HIGH &&
             joyY > JOY_DEADZONE_LOW && joyY < JOY_DEADZONE_HIGH) {
    currentJoyState = STATE_NEUTRAL;
  }

  // Only signal when we move into a directional state (ignore neutral jitter)
  bool stateChanged = (currentJoyState != STATE_NEUTRAL && currentJoyState != lastJoyState);

  lastJoyState = currentJoyState;
  return stateChanged;

}

bool InputHandler::checkResetButton() {
  bool stateChanged = false;
  currentResetState = digitalRead(RESET_BUTTON_PIN);

  if (currentResetState == LOW && lastResetState == HIGH) {
    if (millis() - lastResetPressTime > debounceDelay) {
      lastResetPressTime = millis();
      Serial.println("Reset Button: PRESSED");
      stateChanged = true;
    }
  }

  lastResetState = currentResetState;
  return stateChanged;
}

bool InputHandler::checkSelectButton() {
  bool stateChanged = false;
  currentSelectState = digitalRead(SELECT_BUTTON_PIN);
  if (currentSelectState == LOW && lastSelectState == HIGH) {
    if (millis() - lastSelectPressTime > debounceDelay) {
      lastSelectPressTime = millis();
      Serial.println("Select Button: PRESSED");

      stateChanged = true;
    }
  }

  lastSelectState = currentSelectState;
  return stateChanged;
}

void InputHandler::clearButtonStates() {
  lastSelectState = digitalRead(SELECT_BUTTON_PIN);
  lastResetState = digitalRead(RESET_BUTTON_PIN);
  lastSelectPressTime = millis();
  lastResetPressTime = millis();
}
