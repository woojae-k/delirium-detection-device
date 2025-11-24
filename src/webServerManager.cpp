#include "webServerManager.hpp"

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "storageManager.hpp"

namespace {
const char* kSsid = "CAM_Device";
const char* kMdnsHostname = "cam-device";
constexpr bool kServeExportAssetsFromLittleFs = false;  // set true if you upload /export files to LittleFS

// Fallback assets if LittleFS files are missing
const char defaultStyleCss[] PROGMEM = R"rawliteral(
*{box-sizing:border-box;}
body{font-family:"Segoe UI",Arial,sans-serif;margin:0;padding:32px;background:radial-gradient(circle at 20% 20%,#12325b,#0b1b2b);color:#e9f2ff;}
.page{display:flex;justify-content:center;}
.card{width:100%;max-width:640px;padding:26px;border-radius:16px;background:rgba(19,39,60,0.92);backdrop-filter:blur(4px);
box-shadow:0 18px 40px rgba(0,0,0,0.35);text-align:center;}
.card h1,.card h2{margin:0 0 10px;font-weight:700;}
.card p{margin:10px 0 22px;color:#c8d7ec;}
.actions{display:grid;grid-template-columns:1fr;gap:12px;}
.btn{display:block;width:100%;padding:14px 18px;font-size:17px;color:#0b1b2b;background:#8fffd9;
border:none;border-radius:10px;cursor:pointer;font-weight:700;transition:transform .12s,box-shadow .12s;}
.btn:hover{transform:translateY(-1px);box-shadow:0 8px 20px rgba(0,0,0,0.25);}
.btn.secondary{background:#f57c00;color:#fff;}
.btn.info{background:#4fc3f7;color:#0b1b2b;}
.btn:active{transform:translateY(1px);}
.link{color:#8fffd9;text-decoration:none;font-weight:600;}
.table-wrap{overflow-x:auto;margin-top:14px;}
.data-table{width:100%;border-collapse:collapse;background:#0f1f31;border-radius:12px;overflow:hidden;}
.data-table th,.data-table td{padding:10px 12px;border-bottom:1px solid rgba(255,255,255,0.06);text-align:left;color:#e9f2ff;}
.data-table th{background:#122b44;color:#8fffd9;font-weight:700;}
.data-table tr:nth-child(even){background:rgba(255,255,255,0.03);}
.data-table tr:hover{background:rgba(79,195,247,0.12);}
)rawliteral";

const char defaultIndexHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>CAM Export</title><link rel="stylesheet" href="/style.css">
</head><body class="page"><div class="card">
<h1>Data Export</h1><p>Download or review the latest CAM assessments.</p>
<div class="actions">
<form action="/download" method="get"><button class="btn" type="submit">Download CSV</button></form>
<form action="/view" method="get"><button class="btn info" type="submit">View CSV</button></form>
<form action="/clear" method="get"><button class="btn secondary" type="submit">Delete Stored CSV</button></form>
</div></div></body></html>
)rawliteral";

const char defaultViewHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>CSV Preview</title><link rel="stylesheet" href="/style.css">
</head><body class="page"><div class="card">
<h2>CSV Preview</h2>
<div class="table-wrap"><table class="data-table">{{TABLE_ROWS}}</table></div>
<p style="margin-top:14px;"><a class="link" href="/">Back</a></p>
</div></body></html>
)rawliteral";
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
  IPAddress apIp = WiFi.softAPIP();

  startCaptiveDns(apIp);
  startMdns();

  Serial.println("Export server started");
  Serial.print("SSID: ");
  Serial.println(kSsid);
  Serial.print("IP: ");
  Serial.println(apIp);
  Serial.print("mDNS: http://");
  Serial.print(kMdnsHostname);
  Serial.println(".local/");

  return true;
}

void WebServerManager::configureRoutes() {
  server_.on("/", HTTP_ANY, [this]() { handleRoot(); });
  server_.on("/download", HTTP_ANY, [this]() { handleDownload(); });
  server_.on("/clear", HTTP_ANY, [this]() { handleClear(); });
  server_.on("/view", HTTP_ANY, [this]() { handleView(); });
  server_.on("/style.css", HTTP_ANY, [this]() { handleStyle(); });
  // Handle common connectivity-check endpoints to avoid noisy logs.
  server_.on("/generate_204", HTTP_ANY, [this]() { server_.send(204); });
  server_.on("/gen_204", HTTP_ANY, [this]() { server_.send(204); });
  server_.on("/ncsi.txt", HTTP_ANY, [this]() { server_.send(204); });
  server_.on("/connecttest.txt", HTTP_ANY, [this]() { server_.send(204); });
  server_.onNotFound([this]() {
    sendMessage(404, "Route not found. Try /, /download, /view, or /clear.");
  });
}

void WebServerManager::stopExportServer() {
  if (!serverRunning_) {
    return;
  }

  stopCaptiveDns();
  stopMdns();
  server_.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  serverRunning_ = false;
  Serial.println("Export server stopped");
}

void WebServerManager::loop() {
  if (serverRunning_) {
    if (dnsRunning_) {
      dnsServer_.processNextRequest();
    }
    server_.handleClient();
  }
}

void WebServerManager::handleRoot() {
  if (kServeExportAssetsFromLittleFs) {
    if (streamFileIfExists("/export/index.html", "text/html")) {
      return;
    }
    Serial.println("Serving fallback index.html");
  }
  server_.send_P(200, "text/html", defaultIndexHtml);
}

void WebServerManager::handleDownload() {
  if (!Storage::csvExists()) {
    sendMessage(404, "CSV not found. Go back to / to return home.");
    return;
  }

  File file = LittleFS.open(Storage::csvPath(), "r");
  if (!file) {
    sendMessage(500, "Download failed: could not open the CSV file.");
    return;
  }

  server_.sendHeader("Content-Disposition", "attachment; filename=data.csv");
  server_.streamFile(file, "text/csv");
  file.close();
}

void WebServerManager::handleClear() {
  if (Storage::removeCSV()) {
    sendMessage(200, "CSV removed. Go back to / to return home.");
  } else {
    sendMessage(500, "Delete failed: could not delete the CSV file.");
  }
}

void WebServerManager::handleView() {
  if (!Storage::csvExists()) {
    sendMessage(404, "CSV not found. Go back to / to return home.");
    return;
  }

  File file = LittleFS.open(Storage::csvPath(), "r");
  if (!file) {
    sendMessage(500, "View failed: could not open the CSV file.");
    return;
  }

  bool isHeader = true;
  String rows;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim(); // drop \r
    if (line.length() == 0) continue;

    rows += "<tr>";
    int start = 0;
    while (true) {
      int commaIndex = line.indexOf(',', start);
      String cell = (commaIndex == -1) ? line.substring(start) : line.substring(start, commaIndex);
      // Basic HTML escaping
      cell.replace("&", "&amp;");
      cell.replace("<", "&lt;");
      cell.replace(">", "&gt;");
      cell.replace("\"", "&quot;");

      if (isHeader) {
        rows += "<th>" + cell + "</th>";
      } else {
        rows += "<td>" + cell + "</td>";
      }

      if (commaIndex == -1) break;
      start = commaIndex + 1;
    }
    rows += "</tr>";
    isHeader = false;
  }
  file.close();

  String tmpl = String(defaultViewHtml);
  if (kServeExportAssetsFromLittleFs) {
    String maybeTmpl = loadFileToString("/export/view.html");
    if (!maybeTmpl.isEmpty()) {
      tmpl = maybeTmpl;
    } else {
      Serial.println("Serving fallback view.html");
    }
  }
  tmpl.replace("{{TABLE_ROWS}}", rows);
  server_.send(200, "text/html", tmpl);
}

void WebServerManager::handleStyle() {
  if (kServeExportAssetsFromLittleFs) {
    if (streamFileIfExists("/export/style.css", "text/css")) {
      return;
    }
    Serial.println("Serving fallback style.css");
  }
  server_.send_P(200, "text/css", defaultStyleCss);
}

bool WebServerManager::streamFileIfExists(const char* path, const char* contentType) {
  if (!LittleFS.exists(path)) {
    Serial.printf("Static file missing: %s\n", path);
    return false;
  }
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.printf("Static file missing: %s\n", path);
    return false;
  }
  server_.streamFile(file, contentType);
  file.close();
  return true;
}

String WebServerManager::loadFileToString(const char* path) {
  if (!LittleFS.exists(path)) {
    Serial.printf("Template missing: %s\n", path);
    return String();
  }
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.printf("Template missing: %s\n", path);
    return String();
  }
  String content = file.readString();
  file.close();
  return content;
}

void WebServerManager::sendMessage(int statusCode, const String& message) {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><script>";
  html += "alert('";
  String safeMessage = message;
  safeMessage.replace("\\", "\\\\");
  safeMessage.replace("'", "\\'");
  html += safeMessage;
  html += "');";
  html += "window.location.href='/'";
  html += ";</script></head><body></body></html>";
  server_.send(statusCode, "text/html", html);
}

void WebServerManager::startCaptiveDns(const IPAddress& apIp) {
  // Resolve all domains to our AP IP to mimic a captive portal; helps users who type random URLs.
  dnsRunning_ = dnsServer_.start(53, "*", apIp);
  if (!dnsRunning_) {
    Serial.println("Failed to start DNS server for captive portal");
  }
}

void WebServerManager::stopCaptiveDns() {
  if (dnsRunning_) {
    dnsServer_.stop();
    dnsRunning_ = false;
  }
}

void WebServerManager::startMdns() {
  if (MDNS.begin(kMdnsHostname)) {
    MDNS.addService("http", "tcp", 80);
    mdnsRunning_ = true;
  } else {
    Serial.println("Failed to start mDNS responder");
  }
}

void WebServerManager::stopMdns() {
  if (mdnsRunning_) {
    MDNS.end();
    mdnsRunning_ = false;
  }
}
