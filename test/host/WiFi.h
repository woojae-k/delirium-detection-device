#ifndef HOST_WIFI_H
#define HOST_WIFI_H

#include <Arduino.h>

typedef enum { WL_IDLE_STATUS = 0, WL_CONNECTED = 3, WL_DISCONNECTED = 6 } wl_status_t;
typedef enum { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2 } wifi_mode_t;

namespace hostsim {
inline wl_status_t wifiStatus = WL_CONNECTED;
}

class WiFiClass {
  public:
    wl_status_t status() { return hostsim::wifiStatus; }
    bool mode(wifi_mode_t) { return true; }
    void begin(const char*, const char*) {}
    bool disconnect(bool = false) { return true; }
};

inline WiFiClass WiFi;

#endif
