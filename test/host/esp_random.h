#ifndef HOST_ESP_RANDOM_H
#define HOST_ESP_RANDOM_H

#include <Arduino.h>

inline uint32_t esp_random() { return static_cast<uint32_t>(hostsim::rng()); }

#endif
