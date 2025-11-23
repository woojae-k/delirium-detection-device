#include <unity.h>

#include "../../src/inputHandler.cpp"

namespace {

const int kAdcMax = 4095;  // ESP32 ADC is 12-bit

void setJoystick(int x, int y) {
  hostsim::analogPins[JOYSTICK_X_PIN] = x;
  hostsim::analogPins[JOYSTICK_Y_PIN] = y;
}

void setSelect(int level) { hostsim::digitalPins[SELECT_BUTTON_PIN] = level; }
void setReset(int level) { hostsim::digitalPins[RESET_BUTTON_PIN] = level; }

// Expected mapping from config.hpp: vertical deflection wins over horizontal.
JoystickState expectedState(int x, int y) {
  if (y < JOY_THRESHOLD_LOW) return STATE_UP;
  if (y > JOY_THRESHOLD_HIGH) return STATE_DOWN;
  if (x < JOY_THRESHOLD_LOW) return STATE_LEFT;
  if (x > JOY_THRESHOLD_HIGH) return STATE_RIGHT;
  return STATE_NEUTRAL;
}

int pollEvents(InputHandler& input, int polls) {
  int events = 0;
  for (int i = 0; i < polls; ++i) {
    if (input.checkJoystick()) ++events;
  }
  return events;
}

}  // namespace

void setUp() { hostsim::reset(); }
void tearDown() {}

void test_joystick_classifies_every_adc_reading() {
  InputHandler input;
  input.init();

  long mismatches = 0;
  long samples = 0;
  for (int x = 0; x <= kAdcMax; ++x) {
    for (int y = 0; y <= kAdcMax; ++y) {
      setJoystick(x, y);
      input.checkJoystick();
      if (input.getJoystickState() != expectedState(x, y)) ++mismatches;
      ++samples;
    }
  }
  TEST_ASSERT_EQUAL_INT(4096L * 4096L, samples);
  TEST_ASSERT_EQUAL_INT(0, mismatches);
}

void test_joystick_fires_once_per_deflection() {
  InputHandler input;
  input.init();

  setJoystick(2048, 0);
  TEST_ASSERT_EQUAL_INT(1, pollEvents(input, 100));
  TEST_ASSERT_EQUAL_INT(STATE_UP, input.getJoystickState());

  setJoystick(2048, 2048);
  TEST_ASSERT_EQUAL_INT(0, pollEvents(input, 100));

  setJoystick(2048, 0);
  TEST_ASSERT_EQUAL_INT(1, pollEvents(input, 100));

  // Sliding straight from one direction to another is a new event.
  setJoystick(0, 2048);
  TEST_ASSERT_EQUAL_INT(1, pollEvents(input, 100));
  TEST_ASSERT_EQUAL_INT(STATE_LEFT, input.getJoystickState());
}

void test_select_button_is_debounced() {
  InputHandler input;
  input.init();
  input.clearButtonStates();

  hostsim::nowMs = 1000;
  setSelect(LOW);
  TEST_ASSERT_TRUE(input.checkSelectButton());

  // Holding the button never repeats.
  for (int i = 0; i < 50; ++i) {
    hostsim::nowMs += 10;
    TEST_ASSERT_FALSE(input.checkSelectButton());
  }

  // A bounce/re-press inside the 250 ms window is ignored...
  setSelect(HIGH);
  input.checkSelectButton();
  hostsim::nowMs = 1000 + 250;
  setSelect(LOW);
  TEST_ASSERT_FALSE(input.checkSelectButton());

  // ...but a clean press after the window is accepted.
  setSelect(HIGH);
  input.checkSelectButton();
  hostsim::nowMs = 1000 + 251;
  setSelect(LOW);
  TEST_ASSERT_TRUE(input.checkSelectButton());
}

void test_reset_button_is_debounced() {
  InputHandler input;
  input.init();
  input.clearButtonStates();

  hostsim::nowMs = 500;
  setReset(LOW);
  TEST_ASSERT_TRUE(input.checkResetButton());
  TEST_ASSERT_FALSE(input.checkResetButton());

  setReset(HIGH);
  input.checkResetButton();
  hostsim::nowMs = 600;
  setReset(LOW);
  TEST_ASSERT_FALSE(input.checkResetButton());
}

void test_clear_button_states_swallows_a_held_press() {
  InputHandler input;
  input.init();

  // The press that changed screens is still held when the new screen starts,
  // and is still held after the debounce window has passed.
  hostsim::nowMs = 1000;
  setSelect(LOW);
  input.clearButtonStates();
  hostsim::nowMs += 300;
  for (int i = 0; i < 100; ++i) {
    hostsim::nowMs += 10;
    TEST_ASSERT_FALSE(input.checkSelectButton());
  }

  setSelect(HIGH);
  input.checkSelectButton();
  hostsim::nowMs += 300;
  setSelect(LOW);
  TEST_ASSERT_TRUE(input.checkSelectButton());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_joystick_classifies_every_adc_reading);
  RUN_TEST(test_joystick_fires_once_per_deflection);
  RUN_TEST(test_select_button_is_debounced);
  RUN_TEST(test_reset_button_is_debounced);
  RUN_TEST(test_clear_button_states_swallows_a_held_press);
  return UNITY_END();
}
