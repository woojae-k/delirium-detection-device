// Host-side stand-in for the Arduino core, used only by the native unit tests.
// Implements just enough of String, Print/Serial, GPIO, timing and random()
// for the firmware modules under test to compile and run unchanged on a PC.
#ifndef HOST_ARDUINO_H
#define HOST_ARDUINO_H

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>

#define HIGH 0x1
#define LOW  0x0
#define INPUT        0x01
#define OUTPUT       0x03
#define INPUT_PULLUP 0x05
#define PROGMEM

using std::max;
using std::min;

class String {
  public:
    String(const char* s = "") : s_(s ? s : "") {}
    String(const std::string& s) : s_(s) {}
    explicit String(char c) : s_(1, c) {}
    explicit String(int v) : s_(std::to_string(v)) {}
    explicit String(unsigned long v) : s_(std::to_string(v)) {}

    String& operator=(const char* s) { s_ = s ? s : ""; return *this; }

    const char* c_str() const { return s_.c_str(); }
    unsigned int length() const { return s_.length(); }
    bool isEmpty() const { return s_.empty(); }
    const std::string& str() const { return s_; }
    char operator[](unsigned int i) const { return i < s_.size() ? s_[i] : '\0'; }

    bool concat(const char* s) { if (s) s_ += s; return true; }
    bool concat(const String& s) { s_ += s.s_; return true; }
    String& operator+=(const String& s) { s_ += s.s_; return *this; }
    String& operator+=(const char* s) { if (s) s_ += s; return *this; }
    String& operator+=(char c) { s_ += c; return *this; }

    bool operator==(const String& o) const { return s_ == o.s_; }
    bool operator==(const char* o) const { return s_ == (o ? o : ""); }
    bool operator!=(const String& o) const { return s_ != o.s_; }
    bool startsWith(const String& p) const { return s_.rfind(p.s_, 0) == 0; }
    bool endsWith(const String& p) const {
      return s_.size() >= p.s_.size() && s_.compare(s_.size() - p.s_.size(), p.s_.size(), p.s_) == 0;
    }

    int indexOf(char c, unsigned int from = 0) const { return toIndex(s_.find(c, from)); }
    int indexOf(const String& s, unsigned int from = 0) const { return toIndex(s_.find(s.s_, from)); }
    int lastIndexOf(char c) const { return toIndex(s_.rfind(c)); }

    String substring(unsigned int begin) const { return substring(begin, s_.size()); }
    String substring(unsigned int begin, unsigned int end) const {
      if (begin > end) std::swap(begin, end);
      if (begin >= s_.size()) return String();
      end = std::min<unsigned int>(end, s_.size());
      return String(s_.substr(begin, end - begin));
    }

    void remove(unsigned int index) { if (index < s_.size()) s_.erase(index); }
    void remove(unsigned int index, unsigned int count) {
      if (index < s_.size()) s_.erase(index, std::min<size_t>(count, s_.size() - index));
    }
    void replace(const String& find, const String& repl) {
      if (find.s_.empty()) return;
      size_t pos = 0;
      while ((pos = s_.find(find.s_, pos)) != std::string::npos) {
        s_.replace(pos, find.s_.size(), repl.s_);
        pos += repl.s_.size();
      }
    }
    void trim() {
      size_t b = 0, e = s_.size();
      while (b < e && std::isspace(static_cast<unsigned char>(s_[b]))) ++b;
      while (e > b && std::isspace(static_cast<unsigned char>(s_[e - 1]))) --e;
      s_ = s_.substr(b, e - b);
    }

  private:
    static int toIndex(size_t pos) { return pos == std::string::npos ? -1 : static_cast<int>(pos); }
    std::string s_;
};

inline String operator+(const String& a, const String& b) { return String(a.str() + b.str()); }
inline String operator+(const String& a, const char* b) { return String(a.str() + (b ? b : "")); }
inline String operator+(const char* a, const String& b) { return String(std::string(a ? a : "") + b.str()); }

class Print {
  public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t* buf, size_t n) {
      for (size_t i = 0; i < n; ++i) write(buf[i]);
      return n;
    }

    size_t print(const char* s) { return s ? write(reinterpret_cast<const uint8_t*>(s), strlen(s)) : 0; }
    size_t print(const String& s) { return print(s.c_str()); }
    size_t print(char c) { return write(static_cast<uint8_t>(c)); }
    size_t print(int v) { return print(std::to_string(v).c_str()); }
    size_t print(unsigned int v) { return print(std::to_string(v).c_str()); }
    size_t print(long v) { return print(std::to_string(v).c_str()); }
    size_t print(unsigned long v) { return print(std::to_string(v).c_str()); }

    template <typename T>
    size_t println(const T& v) { return print(v) + println(); }
    size_t println() { return print("\n"); }

    size_t printf(const char* fmt, ...) {
      char buf[512];
      va_list args;
      va_start(args, fmt);
      vsnprintf(buf, sizeof(buf), fmt, args);
      va_end(args);
      return print(buf);
    }
};

class Printable {
  public:
    virtual ~Printable() = default;
    virtual size_t printTo(Print& p) const = 0;
};

// Serial output is captured so tests can assert on what the firmware logs.
class HardwareSerial : public Print {
  public:
    void begin(unsigned long) {}
    size_t write(uint8_t c) override { log_.push_back(static_cast<char>(c)); return 1; }
    using Print::write;
    const std::string& log() const { return log_; }
    void clearLog() { log_.clear(); }

  private:
    std::string log_;
};

inline HardwareSerial Serial;

// Simulated hardware state that tests drive directly.
namespace hostsim {
inline unsigned long nowMs = 0;
inline int digitalPins[64];
inline int analogPins[64];
inline std::mt19937 rng;

inline void reset(uint32_t seed = 12345) {
  nowMs = 0;
  for (int& v : digitalPins) v = HIGH;  // buttons idle high (INPUT_PULLUP)
  for (int& v : analogPins) v = 2048;   // joystick centred on a 12-bit ADC
  rng.seed(seed);
  Serial.clearLog();
}
}  // namespace hostsim

inline unsigned long millis() { return hostsim::nowMs; }
inline void delay(unsigned long ms) { hostsim::nowMs += ms; }
inline void pinMode(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t pin) { return hostsim::digitalPins[pin]; }
inline int analogRead(uint8_t pin) { return hostsim::analogPins[pin]; }
inline void randomSeed(unsigned long seed) { hostsim::rng.seed(static_cast<uint32_t>(seed)); }
inline long random(long howBig) {
  if (howBig <= 0) return 0;
  return std::uniform_int_distribution<long>(0, howBig - 1)(hostsim::rng);
}
inline long random(long howSmall, long howBig) {
  if (howSmall >= howBig) return howSmall;
  return howSmall + random(howBig - howSmall);
}

#endif
