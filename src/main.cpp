#include <Arduino.h>

#include "appController.hpp"

AppController controller;

void setup() { controller.begin(); }

void loop() { controller.loop(); }
