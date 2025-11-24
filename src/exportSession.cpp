#include "exportSession.hpp"

#include <Arduino.h>

ExportSession::ExportSession(WebServerManager& server, DisplayManager& display)
    : webServerManager_(server), displayManager_(display) {}

void ExportSession::start() {
  displayManager_.displayExportInstructions();
  running_ = webServerManager_.startExportServer();
  if (!running_) {
    Serial.println("Failed to start export server");
  }
}

bool ExportSession::loop(InputHandler& input) {
  if (running_) {
    webServerManager_.loop();
  }

  if (input.checkSelectButton()) {
    stop();
    return true;
  }

  return false;
}

void ExportSession::stop() {
  if (webServerManager_.isRunning()) {
    webServerManager_.stopExportServer();
  }
  running_ = false;
}
