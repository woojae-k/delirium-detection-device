#include "storageManager.hpp"

#include <LittleFS.h>

namespace {
constexpr const char* kCsvPath = "/data.csv";
}

namespace Storage {

bool begin() {
  if (LittleFS.begin(true)) {
    return true;
  }
  Serial.println("LittleFS mount failed");
  return false;
}

bool appendResultToCSV(const String& timestamp, int score, const String& riskLevel,
                       const String& patientCode, int additionalCorrect) {
  File file = LittleFS.open(kCsvPath, "a");
  if (!file) {
    Serial.println("Failed to open CSV for append");
    return false;
  }

  file.printf("%s,%s,%d,%d,%s\n", timestamp.c_str(), patientCode.c_str(), score,
              additionalCorrect, riskLevel.c_str());
  file.close();
  return true;
}

bool removeCSV() {
  if (!LittleFS.exists(kCsvPath)) {
    return true;
  }
  return LittleFS.remove(kCsvPath);
}

bool csvExists() { return LittleFS.exists(kCsvPath); }

const char* csvPath() { return kCsvPath; }

}  // namespace Storage
