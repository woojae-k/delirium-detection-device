#include <unity.h>

#include <string>
#include <vector>

#include "../../src/assessmentSession.cpp"
#include "../../src/inputHandler.cpp"
#include "../../src/questionManager.cpp"

// Recording fakes for the TFT display and LittleFS-backed CSV log.
namespace fake {
std::vector<Question> shown;
std::vector<int> highlights;
std::vector<std::string> completionCodes;
struct Row {
  int score;
  std::string risk;
  std::string patientCode;
};
std::vector<Row> rows;

void reset() {
  shown.clear();
  highlights.clear();
  completionCodes.clear();
  rows.clear();
}
}  // namespace fake

void DisplayManager::showQuestion(const Question& question, int selectedChoice) {
  fake::shown.push_back(question);
  fake::highlights.push_back(selectedChoice);
}
void DisplayManager::highlightChoice(int selectedChoice) { fake::highlights.push_back(selectedChoice); }
void DisplayManager::displayCompletionScreen(const String& patientCode) {
  fake::completionCodes.push_back(patientCode.str());
}
bool Storage::appendResultToCSV(int score, const String& riskLevel, const String& patientCode) {
  fake::rows.push_back({score, riskLevel.str(), patientCode.str()});
  return true;
}

namespace {

const unsigned long kTimeoutMs = 10000;
const int kCenter = 2048;

struct Rig {
  QuestionManager questions;
  InputHandler input;
  DisplayManager display;
  AssessmentSession session{questions, input, display};
  Rig() { input.init(); }
};

void setJoystick(int x, int y) {
  hostsim::analogPins[JOYSTICK_X_PIN] = x;
  hostsim::analogPins[JOYSTICK_Y_PIN] = y;
}

// Deflect the joystick once and let it spring back to centre.
void nudge(Rig& rig, int x, int y) {
  setJoystick(x, y);
  rig.session.loop();
  setJoystick(kCenter, kCenter);
  rig.session.loop();
}
void nudgeLeft(Rig& rig) { nudge(rig, 0, kCenter); }
void nudgeRight(Rig& rig) { nudge(rig, 4095, kCenter); }
void nudgeUp(Rig& rig) { nudge(rig, kCenter, 0); }
void nudgeDown(Rig& rig) { nudge(rig, kCenter, 4095); }

// Press and release SELECT at the current simulated time.
bool pressSelect(Rig& rig) {
  hostsim::digitalPins[SELECT_BUTTON_PIN] = LOW;
  const bool exited = rig.session.loop();
  hostsim::digitalPins[SELECT_BUTTON_PIN] = HIGH;
  rig.session.loop();
  return exited;
}

// Choose a cell of the 2x2 answer grid (selection starts at cell 0) and
// confirm it after the patient has spent `thinkMs` on the question.
void answer(Rig& rig, int cell, unsigned long thinkMs = 1000) {
  if (cell % 2 == 1) nudgeRight(rig);
  if (cell / 2 == 1) nudgeDown(rig);
  hostsim::nowMs += thinkMs;
  pressSelect(rig);
}

int correctCell() { return fake::shown.back().correctChoice; }
int wrongCell() { return (fake::shown.back().correctChoice + 1) % 4; }

void runQuiz(Rig& rig, int correctAnswers) {
  rig.session.start();
  for (int i = 0; i < 10; ++i) answer(rig, i < correctAnswers ? correctCell() : wrongCell());
}

int occurrences(const std::string& haystack, const std::string& needle) {
  int n = 0;
  for (size_t pos = haystack.find(needle); pos != std::string::npos; pos = haystack.find(needle, pos + 1)) ++n;
  return n;
}

}  // namespace

void setUp() {
  hostsim::reset();
  fake::reset();
}
void tearDown() {}

void test_perfect_score_is_low_risk_and_logged_once() {
  Rig rig;
  runQuiz(rig, 10);

  TEST_ASSERT_EQUAL_INT(10, fake::shown.size());
  TEST_ASSERT_EQUAL_INT(1, fake::completionCodes.size());
  TEST_ASSERT_EQUAL_STRING("PT-0001", fake::completionCodes[0].c_str());
  TEST_ASSERT_EQUAL_INT(1, fake::rows.size());
  TEST_ASSERT_EQUAL_INT(10, fake::rows[0].score);
  TEST_ASSERT_EQUAL_STRING("Low", fake::rows[0].risk.c_str());
  TEST_ASSERT_EQUAL_STRING("PT-0001", fake::rows[0].patientCode.c_str());

  // The completion screen stays up until SELECT, and the result is not re-logged.
  for (int i = 0; i < 100; ++i) {
    hostsim::nowMs += 100;
    TEST_ASSERT_FALSE(rig.session.loop());
  }
  TEST_ASSERT_TRUE(pressSelect(rig));
  TEST_ASSERT_EQUAL_INT(1, fake::rows.size());
}

void test_all_wrong_answers_are_elevated_risk() {
  Rig rig;
  runQuiz(rig, 0);
  TEST_ASSERT_EQUAL_INT(1, fake::rows.size());
  TEST_ASSERT_EQUAL_INT(0, fake::rows[0].score);
  TEST_ASSERT_EQUAL_STRING("Elevated", fake::rows[0].risk.c_str());
}

void test_risk_cutoff_is_half_of_questions_and_patient_codes_increment() {
  Rig rig;
  runQuiz(rig, 5);
  rig.session.reset();
  hostsim::nowMs += 1000;
  runQuiz(rig, 4);

  TEST_ASSERT_EQUAL_INT(2, fake::rows.size());
  TEST_ASSERT_EQUAL_INT(5, fake::rows[0].score);
  TEST_ASSERT_EQUAL_STRING("Low", fake::rows[0].risk.c_str());
  TEST_ASSERT_EQUAL_STRING("PT-0001", fake::rows[0].patientCode.c_str());
  TEST_ASSERT_EQUAL_INT(4, fake::rows[1].score);
  TEST_ASSERT_EQUAL_STRING("Elevated", fake::rows[1].risk.c_str());
  TEST_ASSERT_EQUAL_STRING("PT-0002", fake::rows[1].patientCode.c_str());
}

void test_unanswered_question_times_out_after_ten_seconds() {
  Rig rig;
  rig.session.start();

  hostsim::nowMs = kTimeoutMs - 1;
  rig.session.loop();
  TEST_ASSERT_EQUAL_INT(1, fake::shown.size());

  hostsim::nowMs = kTimeoutMs;
  rig.session.loop();
  TEST_ASSERT_EQUAL_INT(2, fake::shown.size());
  TEST_ASSERT_EQUAL_INT(1, occurrences(Serial.log(), "TIMEOUT: Unresponsive/Sleeping"));
}

void test_unattended_device_finishes_with_elevated_risk() {
  Rig rig;
  rig.session.start();
  while (fake::rows.empty() && hostsim::nowMs < 20 * kTimeoutMs) {
    hostsim::nowMs += 50;
    rig.session.loop();
  }

  TEST_ASSERT_EQUAL_INT(1, fake::rows.size());
  TEST_ASSERT_EQUAL_INT(0, fake::rows[0].score);
  TEST_ASSERT_EQUAL_STRING("Elevated", fake::rows[0].risk.c_str());
  TEST_ASSERT_EQUAL_INT(10, occurrences(Serial.log(), "TIMEOUT: Unresponsive/Sleeping"));
  TEST_ASSERT_EQUAL_INT(10 * kTimeoutMs, hostsim::nowMs);
}

void test_slow_answers_are_flagged_but_still_scored() {
  Rig rig;
  rig.session.start();
  answer(rig, correctCell(), 6999);
  TEST_ASSERT_EQUAL_INT(0, occurrences(Serial.log(), "FLAG: Slow response"));
  answer(rig, correctCell(), 7001);
  TEST_ASSERT_EQUAL_INT(1, occurrences(Serial.log(), "FLAG: Slow response"));
  TEST_ASSERT_EQUAL_INT(1, occurrences(Serial.log(), "Latency(ms): 7001"));

  for (int i = 2; i < 10; ++i) answer(rig, correctCell());
  TEST_ASSERT_EQUAL_INT(10, fake::rows[0].score);
}

void test_joystick_navigation_stays_inside_answer_grid() {
  Rig rig;
  rig.session.start();
  TEST_ASSERT_EQUAL_INT(0, fake::highlights.back());

  nudgeLeft(rig);
  nudgeUp(rig);
  TEST_ASSERT_EQUAL_INT(0, fake::highlights.back());
  nudgeRight(rig);
  TEST_ASSERT_EQUAL_INT(1, fake::highlights.back());
  nudgeRight(rig);
  TEST_ASSERT_EQUAL_INT(1, fake::highlights.back());
  nudgeDown(rig);
  TEST_ASSERT_EQUAL_INT(3, fake::highlights.back());
  nudgeDown(rig);
  TEST_ASSERT_EQUAL_INT(3, fake::highlights.back());
  nudgeLeft(rig);
  TEST_ASSERT_EQUAL_INT(2, fake::highlights.back());
  nudgeUp(rig);
  TEST_ASSERT_EQUAL_INT(0, fake::highlights.back());

  // Answering resets the cursor to the first cell for the next question.
  answer(rig, 3);
  TEST_ASSERT_EQUAL_INT(2, fake::shown.size());
  TEST_ASSERT_EQUAL_INT(0, fake::highlights.back());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_perfect_score_is_low_risk_and_logged_once);
  RUN_TEST(test_all_wrong_answers_are_elevated_risk);
  RUN_TEST(test_risk_cutoff_is_half_of_questions_and_patient_codes_increment);
  RUN_TEST(test_unanswered_question_times_out_after_ten_seconds);
  RUN_TEST(test_unattended_device_finishes_with_elevated_risk);
  RUN_TEST(test_slow_answers_are_flagged_but_still_scored);
  RUN_TEST(test_joystick_navigation_stays_inside_answer_grid);
  return UNITY_END();
}
