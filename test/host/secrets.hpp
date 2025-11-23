// Fallback credentials for native tests. config.hpp picks up config/secrets.hpp
// first when it exists; the HTTP client is simulated, so nothing is ever sent.
#ifndef HOST_SECRETS_HPP
#define HOST_SECRETS_HPP

#define WIFI_SSID      "test-ssid"
#define WIFI_PASSWORD  "test-password"
#define GEMINI_API_KEY "test-api-key"
#define GEMINI_MODEL   "gemini-test-model"

#endif
