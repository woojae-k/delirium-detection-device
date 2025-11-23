#include <unity.h>

#include <map>
#include <set>
#include <string>

#include "../../src/questionManager.cpp"

namespace {

// Answer key for the built-in CAM question bank, written independently of the
// source so that a typo in either the bank or a shuffle shows up as a failure.
const std::map<std::string, std::string> kAnswerKey = {
    {"Which season has snow?", "Winter"},
    {"What kind of place is this?", "Hospital or Home"},
    {"When do you eat breakfast?", "Morning"},
    {"What is 'FISH' spelled backwards?", "HSIF"},
    {"Which number is the largest?", "102"},
    {"What is 20 minus 3?", "17"},
    {"How many 'A's in S-A-V-E-A-H-A?", "3"},
    {"Will a stone float on water?", "No, it sinks"},
    {"Where do fish live?", "In the water"},
    {"What is a hammer used for?", "Hitting nails"},
};

const int kSessions = 10000;

// Chi-square critical values at alpha = 0.001.
const float kChiSquareCritDf3 = 16.266f;
const float kChiSquareCritDf9 = 27.877f;

float chiSquare(const std::vector<long>& observed) {
  long total = 0;
  for (long o : observed) total += o;
  const float expected = static_cast<float>(total) / observed.size();
  float chi = 0;
  for (long o : observed) chi += (o - expected) * (o - expected) / expected;
  return chi;
}

}  // namespace

void setUp() { hostsim::reset(); }
void tearDown() {}

void test_default_bank_is_well_formed() {
  QuestionManager qm;
  TEST_ASSERT_EQUAL_INT(10, qm.getQuestionCount());

  for (int i = 0; i < qm.getQuestionCount(); ++i) {
    const Question& q = qm.getCurrentQuestion();
    TEST_ASSERT_TRUE(kAnswerKey.count(q.prompt.str()) == 1);
    TEST_ASSERT_TRUE(q.correctChoice >= 0 && q.correctChoice <= 3);

    std::set<std::string> distinct;
    for (const String& c : q.choices) {
      TEST_ASSERT_FALSE(c.isEmpty());
      distinct.insert(c.str());
    }
    TEST_ASSERT_EQUAL_INT(4, distinct.size());
    qm.nextQuestion();
  }
  TEST_ASSERT_TRUE(qm.isQuizComplete());
}

void test_every_session_presents_each_question_exactly_once() {
  QuestionManager qm;
  for (int s = 0; s < kSessions; ++s) {
    qm.reset();
    std::set<std::string> seen;
    while (!qm.isQuizComplete()) {
      seen.insert(qm.getCurrentQuestion().prompt.str());
      qm.nextQuestion();
    }
    TEST_ASSERT_EQUAL_INT(kAnswerKey.size(), seen.size());
  }
}

void test_choice_shuffle_preserves_answer_key() {
  QuestionManager qm;
  long checked = 0;
  for (int s = 0; s < kSessions; ++s) {
    qm.reset();
    while (!qm.isQuizComplete()) {
      const Question& q = qm.getCurrentQuestion();
      TEST_ASSERT_EQUAL_STRING(kAnswerKey.at(q.prompt.str()).c_str(), q.choices[q.correctChoice].c_str());
      ++checked;
      qm.nextQuestion();
    }
  }
  TEST_ASSERT_EQUAL_INT(kSessions * 10, checked);
}

void test_answer_position_and_question_order_are_unbiased() {
  QuestionManager qm;
  std::vector<long> answerSlot(4, 0);
  std::map<std::string, long> firstPrompt;
  for (int s = 0; s < kSessions; ++s) {
    qm.reset();
    ++firstPrompt[qm.getCurrentQuestion().prompt.str()];
    while (!qm.isQuizComplete()) {
      ++answerSlot[qm.getCurrentQuestion().correctChoice];
      qm.nextQuestion();
    }
  }

  std::vector<long> firstCounts;
  for (const auto& kv : firstPrompt) firstCounts.push_back(kv.second);
  TEST_ASSERT_EQUAL_INT(10, firstCounts.size());

  TEST_ASSERT_LESS_THAN_FLOAT(kChiSquareCritDf3, chiSquare(answerSlot));
  TEST_ASSERT_LESS_THAN_FLOAT(kChiSquareCritDf9, chiSquare(firstCounts));
}

void test_progression_stops_cleanly_at_end_of_quiz() {
  QuestionManager qm;
  for (int i = 0; i < 10; ++i) {
    TEST_ASSERT_FALSE(qm.isQuizComplete());
    TEST_ASSERT_EQUAL_INT(i, qm.getCurrentQuestionIndex());
    qm.nextQuestion();
  }
  TEST_ASSERT_TRUE(qm.isQuizComplete());

  qm.nextQuestion();
  qm.nextQuestion();
  TEST_ASSERT_TRUE(qm.isQuizComplete());
  TEST_ASSERT_EQUAL_INT(10, qm.getCurrentQuestionIndex());
}

void test_update_and_restore_question_bank() {
  QuestionManager qm;
  std::vector<Question> custom = {
      {"Custom A?", {"a1", "a2", "a3", "a4"}, 2},
      {"Custom B?", {"b1", "b2", "b3", "b4"}, 0},
      {"Custom C?", {"c1", "c2", "c3", "c4"}, 3},
  };

  const std::map<std::string, std::string> customKey = {
      {"Custom A?", "a3"}, {"Custom B?", "b1"}, {"Custom C?", "c4"}};

  qm.updateQuestions(custom);
  TEST_ASSERT_EQUAL_INT(3, qm.getQuestionCount());
  TEST_ASSERT_EQUAL_INT(0, qm.getCurrentQuestionIndex());
  std::set<std::string> seen;
  while (!qm.isQuizComplete()) {
    const Question& q = qm.getCurrentQuestion();
    seen.insert(q.prompt.str());
    TEST_ASSERT_EQUAL_STRING(customKey.at(q.prompt.str()).c_str(), q.choices[q.correctChoice].c_str());
    qm.nextQuestion();
  }
  TEST_ASSERT_EQUAL_INT(3, seen.size());

  qm.restoreDefaultQuestions();
  TEST_ASSERT_EQUAL_INT(10, qm.getQuestionCount());
  TEST_ASSERT_TRUE(kAnswerKey.count(qm.getCurrentQuestion().prompt.str()) == 1);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_default_bank_is_well_formed);
  RUN_TEST(test_every_session_presents_each_question_exactly_once);
  RUN_TEST(test_choice_shuffle_preserves_answer_key);
  RUN_TEST(test_answer_position_and_question_order_are_unbiased);
  RUN_TEST(test_progression_stops_cleanly_at_end_of_quiz);
  RUN_TEST(test_update_and_restore_question_bank);
  return UNITY_END();
}
