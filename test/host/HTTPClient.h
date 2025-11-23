#ifndef HOST_HTTP_CLIENT_H
#define HOST_HTTP_CLIENT_H

#include <map>
#include <string>

#include <Arduino.h>

// Records the outgoing request and replays a canned response.
namespace hostsim {
struct HttpExchange {
  std::string url;
  std::map<std::string, std::string> headers;
  std::string requestBody;
  int postCount = 0;
  int responseCode = 200;
  std::string responseBody;
};
inline HttpExchange http;
}  // namespace hostsim

class HTTPClient {
  public:
    bool begin(const String& url) { hostsim::http.url = url.str(); return true; }
    void addHeader(const String& name, const String& value) { hostsim::http.headers[name.str()] = value.str(); }
    int POST(const String& body) {
      hostsim::http.requestBody = body.str();
      ++hostsim::http.postCount;
      return hostsim::http.responseCode;
    }
    String getString() { return String(hostsim::http.responseBody); }
    void end() {}
};

#endif
