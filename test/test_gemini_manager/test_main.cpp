#include <unity.h>

#include <string>
#include <vector>

#include "../../src/geminiManager.cpp"

namespace {

const size_t kMaxPromptLen = 80;
const size_t kMaxChoiceLen = 22;
const int kFuzzCases = 10000;

// Wraps the model's text output in the generateContent response envelope.
std::string envelope(const std::string& modelText) {
  JsonDocument doc;
  doc["candidates"][0]["content"]["parts"][0]["text"] = modelText;
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string questionsJson(int count, int firstId = 0) {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < count; ++i) {
    JsonObject q = arr.add<JsonObject>();
    q["prompt"] = "Question " + std::to_string(firstId + i) + "?";
    JsonArray choices = q["choices"].to<JsonArray>();
    for (int c = 0; c < 4; ++c) choices.add("Choice " + std::to_string(c));
    q["correctChoice"] = i % 4;
  }
  std::string out;
  serializeJson(doc, out);
  return out;
}

void respondWith(const std::string& body, int code = 200) {
  hostsim::http.responseCode = code;
  hostsim::http.responseBody = body;
}

std::vector<Question> sentinel() { return {{"untouched", {"a", "b", "c", "d"}, 1}}; }

void assertUntouched(const std::vector<Question>& qs) {
  TEST_ASSERT_EQUAL_INT(1, qs.size());
  TEST_ASSERT_EQUAL_STRING("untouched", qs[0].prompt.c_str());
}

void assertDisplayable(const std::vector<Question>& qs) {
  TEST_ASSERT_EQUAL_INT(10, qs.size());
  for (const Question& q : qs) {
    TEST_ASSERT_TRUE(q.prompt.length() <= kMaxPromptLen);
    for (const String& c : q.choices) TEST_ASSERT_TRUE(c.length() <= kMaxChoiceLen);
    TEST_ASSERT_TRUE(q.correctChoice >= 0 && q.correctChoice <= 3);
  }
}

}  // namespace

void setUp() {
  hostsim::reset();
  hostsim::wifiStatus = WL_CONNECTED;
  hostsim::http = hostsim::HttpExchange();
}
void tearDown() {}

void test_parses_well_formed_response() {
  respondWith(envelope(questionsJson(10)));
  std::vector<Question> qs;
  GeminiManager gemini;

  TEST_ASSERT_TRUE(gemini.fetchQuestions(qs));
  TEST_ASSERT_EQUAL_INT(10, qs.size());
  TEST_ASSERT_EQUAL_STRING("Question 0?", qs[0].prompt.c_str());
  TEST_ASSERT_EQUAL_STRING("Choice 3", qs[9].choices[3].c_str());
  TEST_ASSERT_EQUAL_INT(1, qs[9].correctChoice);
}

void test_request_is_valid_json_for_configured_model() {
  respondWith(envelope(questionsJson(10)));
  std::vector<Question> qs;
  GeminiManager gemini;
  gemini.fetchQuestions(qs);

  TEST_ASSERT_EQUAL_INT(1, hostsim::http.postCount);
  TEST_ASSERT_EQUAL_STRING(
      "https://generativelanguage.googleapis.com/v1beta/models/" GEMINI_MODEL ":generateContent",
      hostsim::http.url.c_str());
  TEST_ASSERT_EQUAL_STRING(GEMINI_API_KEY, hostsim::http.headers["X-goog-api-key"].c_str());
  TEST_ASSERT_EQUAL_STRING("application/json", hostsim::http.headers["Content-Type"].c_str());

  JsonDocument body;
  TEST_ASSERT_TRUE(deserializeJson(body, hostsim::http.requestBody) == DeserializationError::Ok);
  const char* prompt = body["contents"][0]["parts"][0]["text"];
  TEST_ASSERT_NOT_NULL(prompt);
  TEST_ASSERT_NOT_NULL(strstr(prompt, "Return ONLY a raw JSON array of 10 objects"));
}

void test_strips_markdown_code_fences() {
  respondWith(envelope("```json\n" + questionsJson(10) + "\n```\n"));
  std::vector<Question> qs;
  GeminiManager gemini;
  TEST_ASSERT_TRUE(gemini.fetchQuestions(qs));
  TEST_ASSERT_EQUAL_INT(10, qs.size());
}

void test_keeps_exactly_ten_questions() {
  respondWith(envelope(questionsJson(15)));
  std::vector<Question> qs;
  GeminiManager gemini;
  TEST_ASSERT_TRUE(gemini.fetchQuestions(qs));
  TEST_ASSERT_EQUAL_INT(10, qs.size());
  TEST_ASSERT_EQUAL_STRING("Question 9?", qs[9].prompt.c_str());
}

void test_rejects_fewer_than_ten_usable_questions() {
  respondWith(envelope(questionsJson(9)));
  std::vector<Question> qs = sentinel();
  GeminiManager gemini;
  TEST_ASSERT_FALSE(gemini.fetchQuestions(qs));
  assertUntouched(qs);
}

void test_skips_items_missing_fields_or_choices() {
  const std::string good = questionsJson(10);
  const std::string bad = R"({"choices":["a","b","c","d"],"correctChoice":0},)"
                          R"({"prompt":"Too few?","choices":["a","b","c"],"correctChoice":0},)"
                          R"({"prompt":"No choices?","correctChoice":0},)";
  respondWith(envelope("[" + bad + good.substr(1)));
  std::vector<Question> qs;
  GeminiManager gemini;
  TEST_ASSERT_TRUE(gemini.fetchQuestions(qs));
  assertDisplayable(qs);
  TEST_ASSERT_EQUAL_STRING("Question 0?", qs[0].prompt.c_str());

  // Dropping the malformed items leaves only 9 usable questions.
  respondWith(envelope("[" + bad + questionsJson(9).substr(1)));
  qs = sentinel();
  TEST_ASSERT_FALSE(gemini.fetchQuestions(qs));
  assertUntouched(qs);
}

void test_sanitizes_text_to_fit_display_and_clamps_answer_index() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < 10; ++i) {
    JsonObject q = arr.add<JsonObject>();
    q["prompt"] = "   " + std::string(200, 'P') + "   ";
    JsonArray choices = q["choices"].to<JsonArray>();
    choices.add("  short  ");
    choices.add(std::string(50, 'C'));
    choices.add("x");
    choices.add("y");
    q["correctChoice"] = (i % 2 == 0) ? 7 : -1;
  }
  std::string inner;
  serializeJson(doc, inner);
  respondWith(envelope(inner));

  std::vector<Question> qs;
  GeminiManager gemini;
  TEST_ASSERT_TRUE(gemini.fetchQuestions(qs));
  assertDisplayable(qs);
  for (const Question& q : qs) {
    TEST_ASSERT_EQUAL_INT(kMaxPromptLen, q.prompt.length());
    TEST_ASSERT_TRUE(q.prompt.endsWith("..."));
    TEST_ASSERT_EQUAL_STRING("short", q.choices[0].c_str());
    TEST_ASSERT_EQUAL_INT(kMaxChoiceLen, q.choices[1].length());
    TEST_ASSERT_EQUAL_INT(0, q.correctChoice);
  }
}

void test_rejects_malformed_responses_without_touching_questions() {
  const std::vector<std::string> bodies = {
      "",
      "not json",
      R"({"error":{"code":429,"message":"Resource exhausted"}})",
      R"({"candidates":[]})",
      R"({"candidates":[{"content":{"parts":[{}]}}]})",
      envelope("Sure! Here are your questions:"),
      envelope(R"({"prompt":"an object, not an array"})"),
      envelope(questionsJson(10).substr(0, 200)),
  };
  GeminiManager gemini;
  for (const std::string& body : bodies) {
    respondWith(body);
    std::vector<Question> qs = sentinel();
    TEST_ASSERT_FALSE(gemini.fetchQuestions(qs));
    assertUntouched(qs);
  }
}

void test_http_failure_returns_false() {
  respondWith("", -1);  // connection refused / timeout
  std::vector<Question> qs = sentinel();
  GeminiManager gemini;
  TEST_ASSERT_FALSE(gemini.fetchQuestions(qs));
  assertUntouched(qs);
}

void test_no_request_when_wifi_is_down() {
  hostsim::wifiStatus = WL_DISCONNECTED;
  respondWith(envelope(questionsJson(10)));
  std::vector<Question> qs = sentinel();
  GeminiManager gemini;
  TEST_ASSERT_FALSE(gemini.fetchQuestions(qs));
  TEST_ASSERT_EQUAL_INT(0, hostsim::http.postCount);
  assertUntouched(qs);
}

// Feeds deterministic, randomly corrupted model output through the real parser.
// Whatever comes back must either be rejected outright (questions untouched) or
// be exactly ten questions that fit the screen with a valid answer index.
void test_fuzzed_responses_never_yield_undisplayable_questions() {
  std::mt19937 gen(20251012);
  auto pick = [&](int n) { return static_cast<int>(gen() % n); };
  auto randomText = [&](int maxLen) {
    std::string s(pick(maxLen + 1), ' ');
    for (char& c : s) c = static_cast<char>(32 + pick(95));
    return s;
  };
  auto randomValue = [&](JsonVariant v) {
    switch (pick(6)) {
      case 0: v.set(randomText(120)); break;
      case 1: v.set(pick(2000) - 1000); break;
      case 2: v.set(pick(1000) / 7.0); break;
      case 3: v.set(nullptr); break;
      case 4: v.to<JsonArray>().add(randomText(5)); break;
      default: v.to<JsonObject>()["k"] = randomText(5); break;
    }
  };
  auto mutateBytes = [&](std::string s) {
    for (int edits = 1 + pick(5); edits > 0 && !s.empty(); --edits) {
      const size_t at = pick(s.size());
      switch (pick(3)) {
        case 0: s[at] = static_cast<char>(pick(256)); break;
        case 1: s.insert(at, 1, "{}[],:\"\\`"[pick(9)]); break;
        default: s.erase(at, 1); break;
      }
    }
    return s;
  };

  GeminiManager gemini;
  int accepted = 0;
  int rejected = 0;
  for (int i = 0; i < kFuzzCases; ++i) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    const int count = 8 + pick(8);
    for (int n = 0; n < count; ++n) {
      JsonObject q = arr.add<JsonObject>();
      if (pick(10)) q["prompt"] = randomText(150); else randomValue(q["prompt"]);
      if (pick(10)) {
        JsonArray choices = q["choices"].to<JsonArray>();
        const int choiceCount = pick(10) ? 4 : pick(7);
        for (int c = 0; c < choiceCount; ++c) {
          if (pick(10)) choices.add(randomText(40)); else randomValue(choices.add<JsonVariant>());
        }
      } else {
        randomValue(q["choices"]);
      }
      if (pick(10)) q["correctChoice"] = pick(12) - 4; else if (pick(2)) randomValue(q["correctChoice"]);
    }
    std::string inner;
    serializeJson(doc, inner);

    std::string body;
    switch (pick(5)) {
      case 0: body = envelope(inner); break;
      case 1: body = envelope("```json\n" + inner + "\n```"); break;
      case 2: body = envelope(mutateBytes(inner)); break;
      case 3: body = mutateBytes(envelope(inner)); break;
      default: body = envelope(inner).substr(0, pick(envelope(inner).size() + 1)); break;
    }

    respondWith(body);
    Serial.clearLog();
    std::vector<Question> qs = sentinel();
    if (gemini.fetchQuestions(qs)) {
      assertDisplayable(qs);
      ++accepted;
    } else {
      assertUntouched(qs);
      ++rejected;
    }
  }

  char summary[96];
  snprintf(summary, sizeof(summary), "fuzz cases=%d accepted=%d rejected=%d", kFuzzCases, accepted,
           rejected);
  TEST_MESSAGE(summary);
  TEST_ASSERT_EQUAL_INT(kFuzzCases, accepted + rejected);
  TEST_ASSERT_GREATER_THAN_INT(0, accepted);
  TEST_ASSERT_GREATER_THAN_INT(0, rejected);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parses_well_formed_response);
  RUN_TEST(test_request_is_valid_json_for_configured_model);
  RUN_TEST(test_strips_markdown_code_fences);
  RUN_TEST(test_keeps_exactly_ten_questions);
  RUN_TEST(test_rejects_fewer_than_ten_usable_questions);
  RUN_TEST(test_skips_items_missing_fields_or_choices);
  RUN_TEST(test_sanitizes_text_to_fit_display_and_clamps_answer_index);
  RUN_TEST(test_rejects_malformed_responses_without_touching_questions);
  RUN_TEST(test_http_failure_returns_false);
  RUN_TEST(test_no_request_when_wifi_is_down);
  RUN_TEST(test_fuzzed_responses_never_yield_undisplayable_questions);
  return UNITY_END();
}
