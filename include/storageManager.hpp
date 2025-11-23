#ifndef STORAGE_MANAGER_HPP
#define STORAGE_MANAGER_HPP

#include <Arduino.h>

namespace Storage {

bool begin();
bool appendResultToCSV(const String& timestamp, int score, const String& riskLevel,
                       const String& patientCode);
bool removeCSV();
bool csvExists();
const char* csvPath();

}  // namespace Storage

#endif
