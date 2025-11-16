#ifndef CONFIG_HPP
#define CONFIG_HPP

// PIN DEFINITIONS 
#define SELECT_BUTTON_PIN 21 // Button to confirm choice
#define RESET_BUTTON_PIN  22 // Button to reset the device
#define JOYSTICK_X_PIN    33 // Analog pin for Joystick X-axis
// End of Pin Definitions

// Joystick analog thresholds (Tune these by checking Serial Monitor)
#define JOY_THRESHOLD_LOW   1000
#define JOY_THRESHOLD_HIGH  3000
#define JOY_DEADZONE_LOW    1800
#define JOY_DEADZONE_HIGH   2200

// FONT DEFINITIONS
#define LOAD_FONT2
#define LOAD_FONT4

// End of Pin Definitions

#endif
