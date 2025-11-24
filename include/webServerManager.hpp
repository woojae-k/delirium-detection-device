#ifndef WEB_SERVER_MANAGER_HPP
#define WEB_SERVER_MANAGER_HPP

#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

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
    void handleStyle();
    bool streamFileIfExists(const char* path, const char* contentType);
    String loadFileToString(const char* path);
    void startCaptiveDns(const IPAddress& apIp);
    void stopCaptiveDns();
    void startMdns();
    void stopMdns();
    void sendMessage(int statusCode, const String& message);

    WebServer server_{80};
    DNSServer dnsServer_;
    bool serverRunning_{false};
    bool dnsRunning_{false};
    bool mdnsRunning_{false};
};

#endif
