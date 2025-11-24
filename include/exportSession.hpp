#ifndef EXPORT_SESSION_HPP
#define EXPORT_SESSION_HPP

#include "displayManager.hpp"
#include "inputHandler.hpp"
#include "webServerManager.hpp"

class ExportSession {
  public:
    ExportSession(WebServerManager& server, DisplayManager& display);

    void start();
    // Returns true when the user presses SELECT to exit export mode.
    bool loop(InputHandler& input);
    void stop();

  private:
    WebServerManager& webServerManager_;
    DisplayManager& displayManager_;
    bool running_{false};
};

#endif
