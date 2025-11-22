#ifndef WEB_SERVER_MANAGER_HPP
#define WEB_SERVER_MANAGER_HPP

#include <WebServer.h>

class WebServerManager {
  public:
    bool startExportServer();
    void stopExportServer();
    void loop();
    bool isRunning() const { return serverRunning_; }

  private:
    void configureRoutes();
    void handleRoot();
    void handleDownload();
    void handleClear();
    void handleView();

    WebServer server_{80};
    bool serverRunning_{false};
};

#endif
