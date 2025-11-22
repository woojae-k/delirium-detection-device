#include "webServerManager.hpp"

#include <LittleFS.h>
#include <WiFi.h>

#include "storageManager.hpp"

namespace {
const char* kSsid = "CAM_Device";
}

bool WebServerManager::startExportServer() {
  if (serverRunning_) {
    return true;
  }

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(kSsid)) {
    Serial.println("Failed to start AP");
    return false;
  }

  configureRoutes();
  server_.begin();
  serverRunning_ = true;

  Serial.println("Export server started");
  Serial.print("SSID: ");
  Serial.println(kSsid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  return true;
}

void WebServerManager::configureRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/download", HTTP_GET, [this]() { handleDownload(); });
  server_.on("/clear", HTTP_GET, [this]() { handleClear(); });
  server_.on("/view", HTTP_GET, [this]() { handleView(); });
}

void WebServerManager::stopExportServer() {
  if (!serverRunning_) {
    return;
  }

  server_.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  serverRunning_ = false;
  Serial.println("Export server stopped");
}

void WebServerManager::loop() {
  if (serverRunning_) {
    server_.handleClient();
  }
}

void WebServerManager::handleRoot() {
  static const char page[] =
      "<!DOCTYPE html>"
      "<html lang='en'>"
      "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'/>"
      "<title>CAM Export</title>"
      "<style>"
      "body{font-family:Arial;margin:0;padding:32px;background:#0b1b2b;color:#f0f4f8;}"
      ".card{max-width:500px;margin:auto;padding:24px;border-radius:14px;background:#13273c;"
      "box-shadow:0 10px 30px rgba(0,0,0,0.3);text-align:center;}"
      "h1{margin-bottom:8px;font-size:26px;}"
      "p{margin:8px 0 20px;}"
      ".btn{display:block;width:100%;padding:14px 18px;margin:10px 0;font-size:18px;"
      "color:#0b1b2b;background:#7fffd4;border:none;border-radius:10px;cursor:pointer;font-weight:700;}"
      ".btn.secondary{background:#f57c00;color:#fff;}"
      ".btn:active{transform:translateY(1px);}"
      "</style></head>"
      "<body><div class='card'><h1>Data Export</h1>"
      "<p>Download the latest assessment CSV or clear stored data.</p>"
      "<form action='/download' method='get'><button class='btn' type='submit'>Download Excel (CSV)</button></form>"
      "<form action='/view' method='get'><button class='btn' type='submit' style='background:#4fc3f7;color:#0b1b2b;'>View CSV</button></form>"
      "<form action='/clear' method='get'><button class='btn secondary' type='submit'>Delete Stored CSV</button></form>"
      "</div></body></html>";

  server_.send(200, "text/html", page);
}

void WebServerManager::handleDownload() {
  if (!Storage::csvExists()) {
    server_.send(404, "text/plain", "CSV not found");
    return;
  }

  File file = LittleFS.open(Storage::csvPath(), "r");
  if (!file) {
    server_.send(500, "text/plain", "Failed to open CSV");
    return;
  }

  server_.sendHeader("Content-Disposition", "attachment; filename=data.csv");
  server_.streamFile(file, "text/csv");
  file.close();
}

void WebServerManager::handleClear() {
  if (Storage::removeCSV()) {
    server_.send(200, "text/plain", "CSV removed");
  } else {
    server_.send(500, "text/plain", "Failed to remove CSV");
  }
}

void WebServerManager::handleView() {
  if (!Storage::csvExists()) {
    server_.send(404, "text/plain", "CSV not found");
    return;
  }

  File file = LittleFS.open(Storage::csvPath(), "r");
  if (!file) {
    server_.send(500, "text/plain", "Failed to open CSV");
    return;
  }

  String content;
  while (file.available()) {
    content += file.readStringUntil('\n');
    content += "\n";
  }
  file.close();

  String page =
      "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'/>"
      "<title>CSV Preview</title>"
      "<style>body{font-family:Arial;background:#0b1b2b;color:#f0f4f8;padding:20px;}pre{white-space:pre-wrap;"
      "word-wrap:break-word;background:#13273c;padding:12px;border-radius:10px;}</style></head><body>"
      "<h2>CSV Preview</h2><pre>";
  page += content;
  page += "</pre><p><a href='/' style='color:#7fffd4;'>Back</a></p></body></html>";

  server_.send(200, "text/html", page);
}
