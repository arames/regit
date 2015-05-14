#include <initializer_list>

#include <argp.h>
#include <string.h>

#include "checks.h"
#include "globals.h"
#include "regit.h"


// Start the enum from the latest argp key used.
enum regit_flags_option_keys {
  // Hope it does not collide with other keys.
  base_regit_flag_key = 0x7BAD,
#define ENUM_KEYS(flag_name, r, d, desc) flag_name##_key,
  REGIT_FLAGS_LIST(ENUM_KEYS)
#undef ENUM_KEYS
};


struct argp_option options[] =
{
  {"line", 'l', "0", OPTION_ARG_OPTIONAL,
    "Only run the tests from the specified line. (Or 0 to run all tests.)", 0},
  {"test-id", 't', "0", OPTION_ARG_OPTIONAL,
    "Only run the test with the specified id. (Or 0 to run all tests.)", 0},
  {"break_on_fail", 'b', nullptr, OPTION_ARG_OPTIONAL,
    "Break when a test fails.", 0},
  {"verbose", 'v', nullptr, OPTION_ARG_OPTIONAL,
    "Print the line and test-id of the tests run.", 0},
  // Convenient access to regit flags.
#define FLAG_OPTION(flag_name, r, d, desc)                                     \
  {#flag_name, flag_name##_key,                                                \
   FLAG_##flag_name ? "1" : "0", OPTION_ARG_OPTIONAL,                          \
   desc "\n0 to disable, 1 to enable.", 0},
  REGIT_FLAGS_LIST(FLAG_OPTION)
#undef FLAG_OPTION
  {0, 0, 0, 0, 0, 0,}
};

char doc[] =
"\n"
"Test a range of regular expressions.\n";

char args_doc[] = "";

const char *argp_program_bug_address = "<alexandre@uop.re>";

struct arguments {
  int line;
  int test_id;
  bool break_on_fail;
  bool verbose;
};
struct arguments arguments;

error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments =
    reinterpret_cast<struct arguments*>(state->input);
  switch (key) {
    case 'b':
      arguments->break_on_fail = true;
      break;
    case 'l':
      if (arg) {
        arguments->line = stol(arg);
      }
      break;
    case 't':
      if (arg) {
        arguments->test_id = stol(arg);
      }
      break;
    case 'v':
      arguments->verbose = true;
      break;
#define FLAG_CASE(flag_name, r, d, desc)                                       \
    case flag_name##_key: {                                                    \
      unsigned v = (arg == nullptr) ? 1 : stol(arg);                           \
      assert(v == 0 || v == 1);                                                \
      FLAG_##flag_name = v;                                                    \
      break;                                                                   \
    }
    REGIT_FLAGS_LIST(FLAG_CASE)
#undef FLAG_CASE

    case ARGP_KEY_ARG:
      argp_usage(state);
      break;
    case ARGP_KEY_END:
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp argp =
    { options, parse_opt, args_doc, doc, nullptr, nullptr, nullptr };


namespace regit {

#define x10(s) s s s s s s s s s s
#define x50(s) x10(s) x10(s) x10(s) x10(s) x10(s)
#define x100(s) x50(s) x50(s)


static bool ShouldTest(const struct arguments *arguments,
                       int line, int test_id) {
  return ((arguments->test_id == 0) || (arguments->test_id == test_id))
      && ((arguments->line == 0) || (arguments->line == line));
}


static void PrintTest(int line, int test_id) {
  printf("Running test line %d test_id %d\n", line, test_id);
}


enum TestStatus {
  TEST_SKIPPED = 0,
  TEST_PASSED = 1,
  TEST_FAILED = -1
};


class TestCounters {
 public:
  TestCounters() : count_passed(0), count_failed(0), count_skipped(0) {}
  int count_passed;
  int count_failed;
  int count_skipped;
};


class TestContext {
 public:
  TestContext(const struct arguments* arguments)
      : arguments_(arguments), test_id_(0) {}
  const struct arguments* arguments_;
  int test_id_;
  TestCounters test_counters_;
};


struct MatchOffsets {
  int start;
  int end;
};

// There are three levels of test helpers.
// - The `DoTest<Type>` helpers perform only one test.
// - The `Test<Type>` helpers perform the test explicitly required, plus
//   additional tests derived from it.
// - The `TEST_<Type>` macros, that hide argument passing for the context and
//   the source-code line.
static void DoTestFull(
    TestContext* context, unsigned line,
    const char* regexp, const string& text,
    bool expected);
static void DoTestAnywhere(
    TestContext* context, unsigned line,
    const char* regexp, const string& text,
    bool expected);
static void DoTestFirst(
    TestContext* context, unsigned line,
    const char* regexp, const string& text,
    bool expected, MatchOffsets expected_match);
static void DoTestAll(
    TestContext* context, unsigned line,
    const char* regexp, const string& text,
    unsigned expected, const std::vector<MatchOffsets>& expected_matches,
    bool only_check_specified_matches = false);

static void TestFull(
    TestContext* context, unsigned line,
    const char* regexp, const string& text,
    bool expected);
static void TestFirst(
    TestContext* context, unsigned line,
    const char* regexp, const string& text,
    bool expected, MatchOffsets expected_match = {-1, -1},
    bool bound = false);
static void TestAll(
    TestContext* context, unsigned line,
    const char* regexp, const string& text,
    const std::vector<MatchOffsets>& expected_matches,
    bool bound = false);



int RunTests(const struct arguments *arguments) {
  TestContext context(arguments);

#define TEST_Full(expected, re, text)                                          \
  TestFull(&context, __LINE__, re, string(text), expected);

#define TEST_First(expected, re, text, ...)                                    \
  TestFirst(&context, __LINE__, re, string(text), expected, __VA_ARGS__);

#define TEST_First_bound(expected, re, text, ...)                              \
  TestFirst(&context, __LINE__, re, string(text), expected,__VA_ARGS__, true);

#define TEST_All(re, text, ...)                                                \
  TestAll(&context, __LINE__, re, string(text), __VA_ARGS__);

#define TEST_All_bound(re, text, ...)                                          \
  TestAll(&context, __LINE__, re, string(text), __VA_ARGS__, true);

  // Basic tests for the helpers.
  TEST_Full(1, "x", "x");
  TEST_Full(0, "x", "y");
  TEST_First(1, "x", "x", {0, 1});
  TEST_First(0, "x", "y", {0, 1});
  TEST_All("x", "x", {{0, 1}});
  TEST_All("x", "y", {});

  TEST_Full(0, "x", "xxxxxx");
  TEST_All("abcdefghij", "abcdefghij", {{0, 10}});
  TEST_Full(0, "abcdefghij", "abcdefghij_klmnop");
  TEST_All("123456789", "123456789", {{0, 9}});
  TEST_All("123456789", "___12345678", {});
  TEST_All("123456789", "_123456789_", {{1, 10}});

  // More characters than can be hold within one MultipleChar.
  TEST_All(x10("abcdefghij"), x10("abcdefghij"), {{0, 100}});
  TEST_Full(0, x10("abcdefghij"), x10("abcdefghij") "X");
  TEST_Full(0, x10("abcdefghij"), "X" x10("abcdefghij"));
  TEST_All(x100("abcdefghij"), x100("abcdefghij"), {{0, 1000}});
  TEST_Full(0, x100("abcdefghij"), x100("abcdefghij") "X");
  TEST_Full(0, x100("abcdefghij"), "X" x100("abcdefghij"));

  // Period.
  TEST_All_bound(".", "x", {{0, 1}});
  TEST_Full(0, ".", "\n");
  TEST_Full(0, ".", "\r");
  TEST_All("abcde.ghij", "abcdefghij", {{0, 10}});
  TEST_Full(0, "abcdefghi.", "abcdefghijklmn");
  TEST_Full(0, "a.b", "a\nb");
  TEST_Full(0, "a.b", "a\rb");
  TEST_All_bound("...", "abc", {{0, 3}});
  TEST_Full(0, "...", "ab");
  TEST_Full(0, "..", "abc");
  TEST_All(x100("abcde.ghij"), x100("abcde.ghij"), {{0, 1000}});
  TEST_Full(0, x100("abcde.ghij"), x100("abcde.ghij") "X");
  TEST_Full(0, x100("abcde.ghij"), "X" x100("abcde.ghij"));
  TEST_All("...123456789", "xxx123456789", {{0, 12}});
  TEST_Full(0, "...123456789", "xx1234567890");

  TEST_First(1, "-", "-\n--\n---\n----\n-----\n------", {0, 1});
  TEST_First(1, "--", "-\n--\n---\n----\n-----\n------", {2, 4});
  TEST_All("---", "-\n--\n---\n----\n-----\n------",
           {{5, 8}, {9, 12}, {14, 17}, {20, 23}, {23, 26}});
  TEST_All("----", "-\n--\n---\n----\n-----\n------",
           {{9, 13}, {14, 18}, {20, 24}});
  TEST_All("-----", "-\n--\n---\n----\n-----\n------",
           {{14, 19}, {20, 25}});
  TEST_All("------", "-\n--\n---\n----\n-----\n------",
           {{20, 26}});
  TEST_All("-------", "-\n--\n---\n----\n-----\n------",
           {});

  // Alternation.
  TEST_All("abcd|efgh", "abcd", {{0, 4}});
  TEST_All("abcd|efgh", "efgh", {{0, 4}});
  TEST_All("abcd|efgh|ijkl", "abcd", {{0, 4}});
  TEST_All("abcd|efgh|ijkl", "efgh", {{0, 4}});
  TEST_All("abcd|efgh|ijkl", "ijkl", {{0, 4}});
  TEST_Full(0, "abcd|efgh|ijkl", "_efgh___");
  TEST_Full(0, "abcd|efgh|ijkl", "abcdefghijkl");
  TEST_Full(0, "abcd|efgh|ijkl", "abcd|efgh|ijkl");

  TEST_All("..(abcX|abcd)..", "..abcd..", {{0, 8}});
  TEST_All("..(abcd|abcX)..", "..abcd..", {{0, 8}});

  TEST_All("(abcX|abcd)", "abcd", {{0, 4}});
  TEST_All("(abcX|abcd)", "abcd..", {{0, 4}});
  TEST_All("(abcX|abcd)", "..abcd", {{2, 6}});
  TEST_All("(abcX|abcd)", "..abcd..", {{2, 6}});
  TEST_All("(abcd|abcX)", "abcd", {{0, 4}});
  TEST_All("(abcd|abcX)", "abcd..", {{0, 4}});
  TEST_All("(abcd|abcX)", "..abcd", {{2, 6}});
  TEST_All("(abcd|abcX)", "..abcd..", {{2, 6}});

  TEST_All("a..b|01", "a01b", {{0, 4}});
  TEST_All("01|a..b", "a01b", {{0, 4}});

  if (context.test_counters_.count_failed) {
      printf("passed: %d\tfailed: %d\tskipped: %d\t(total: %d)\n",
             context.test_counters_.count_passed,
             context.test_counters_.count_failed,
             context.test_counters_.count_skipped,
             context.test_counters_.count_failed
             + context.test_counters_.count_passed
             + context.test_counters_.count_skipped);
  } else {
    printf("success\n");
  }
  return context.test_counters_.count_failed;
}


static bool StartTest(TestContext* context, unsigned line) {
  context->test_id_++;
  if (!ShouldTest(context->arguments_, line, context->test_id_)) {
    context->test_counters_.count_skipped++;
    return false;
  }

  if (context->arguments_->verbose) {
    PrintTest(line, context->test_id_);
  }
  return true;
}


// This does not print a newline to allow the user to extend the 'expected'
// line.
static void ReportFailure(TestContext* context, unsigned line,
                          const char* regexp, const string& text,
                          bool expected) {
  printf("FAILED line %d test_id %d\n"
         "regexp:\n"
         "%s\n"
         "text:\n"
         "%s\n"
         "expected: %d",
         line, context->test_id_,
         regexp,
         text.c_str(),
         expected);
}


static void PrintMatch(int start, int end) {
  printf("(");
  if (start != -1) { printf("%d", start); }
  printf(":");
  if (end != -1) { printf("%d", end); }
  printf(")");
}


static void DoTestFull(TestContext* context, unsigned line,
                       const char* regexp, const string& text,
                       bool expected) {
  if (!StartTest(context, line)) {
    return;
  }

  bool exception_occurred = false;
  bool found = 0;

  try {
    Regit re(regexp);
    found = re.MatchFull(text);
  } catch (int e) {
    exception_occurred = true;
  }

  bool failure = (found != expected) || exception_occurred;

  if (failure) {
    context->test_counters_.count_failed++;
    ReportFailure(context, line, regexp, text, expected);
    printf("\n");
  } else {
    context->test_counters_.count_passed++;
  }

  TestStatus status = failure ? TEST_FAILED : TEST_PASSED;
  assert(!context->arguments_->break_on_fail || (status == TEST_PASSED));
}


static void DoTestAnywhere(TestContext* context, unsigned line,
                           const char* regexp, const string& text,
                           bool expected) {
  if (!StartTest(context, line)) {
    return;
  }

  bool exception_occurred = false;
  bool found = 0;
  Match match;

  try {
    Regit re(regexp);
    found = re.MatchAnywhere(&match, text);
  } catch (int e) {
    exception_occurred = true;
  }

  bool failure = (found != expected) || exception_occurred;

  if (failure) {
    context->test_counters_.count_failed++;
    ReportFailure(context, line, regexp, text, expected);
    printf("\n");
  } else {
    context->test_counters_.count_passed++;
  }

  TestStatus status = failure ? TEST_FAILED : TEST_PASSED;
  assert(!context->arguments_->break_on_fail || (status == TEST_PASSED));
}


static void DoTestFirst(TestContext* context, unsigned line,
                        const char* regexp, const string& text,
                        bool expected, MatchOffsets expected_match) {
  if (!StartTest(context, line)) {
    return;
  }

  bool exception_occurred = false;
  bool incorrect_match = false;
  bool found = 0;
  Match match;

  try {
    Regit re(regexp);
    found = re.MatchFirst(&match, text);
  } catch (int e) {
    exception_occurred = true;
  }

  int found_start = match.start - text.c_str();
  int found_end = match.end - text.c_str();
  if (expected == true) {
    incorrect_match |=
        (expected_match.start != -1) && found_start != expected_match.start;
    incorrect_match |=
        (expected_match.end != -1) && found_end != expected_match.end;
  }

  bool failure = (found != expected) || incorrect_match || exception_occurred;

  if (failure) {
    context->test_counters_.count_failed++;
    ReportFailure(context, line, regexp, text, expected);
    printf(" ");
    PrintMatch(expected_match.start, expected_match.end);
    printf("\n");
    printf("found: %d ", found);
    PrintMatch(found_start, found_end);
    printf("\n");
  } else {
    context->test_counters_.count_passed++;
  }

  TestStatus status = failure ? TEST_FAILED : TEST_PASSED;
  assert(!context->arguments_->break_on_fail || (status == TEST_PASSED));
}


static void DoTestAll(TestContext* context, unsigned line,
                      const char* regexp, const string& text,
                      unsigned expected,
                      const std::vector<MatchOffsets>& expected_matches,
                      bool only_check_specified_matches) {
  if (!StartTest(context, line)) {
    return;
  }

  bool exception_occurred = false;
  bool incorrect_match = false;
  vector<Match> matches;

  try {
    Regit re(regexp);
    re.MatchAll(&matches, text);
  } catch (int e) {
    exception_occurred = true;
  }

  int expected_start;
  int expected_end;
  int found_start;
  int found_end;
  for (unsigned i = 0; i < matches.size(); i++) {
    if (i >= expected && only_check_specified_matches) {
      break;
    }
    expected_start = expected_matches[i].start;
    expected_end = expected_matches[i].end;
    found_start = matches[i].start - text.c_str();
    found_end = matches[i].end - text.c_str();
    incorrect_match |= (expected_start != -1) && found_start != expected_start;
    incorrect_match |= (expected_end != -1) && found_end != expected_end;
    if (incorrect_match) {
      break;
    }
  }
  
  unsigned found = matches.size();
  bool correct_number_of_matches =
      (expected == found) ||
      (only_check_specified_matches && (found >= expected));

  bool failure =
      !correct_number_of_matches || incorrect_match || exception_occurred;

  if (failure) {
    context->test_counters_.count_failed++;
    ReportFailure(context, line, regexp, text, expected);
    for (unsigned i = 0; i < expected; i++) {
      printf(" ");
      PrintMatch(expected_matches[i].start, expected_matches[i].end);
    }
    printf("\n");
    printf("found: %u ", found);
    for (unsigned i = 0; i < matches.size(); i++) {
      printf(" ");
      PrintMatch(matches[i].start - text.c_str(),
                 matches[i].end - text.c_str());
    }
    printf("\n");
  } else {
    context->test_counters_.count_passed++;
  }

  TestStatus status = failure ? TEST_FAILED : TEST_PASSED;
  assert(!context->arguments_->break_on_fail || (status == TEST_PASSED));
}


static void TestFull(TestContext* context, unsigned line,
                     const char* regexp, const string& text,
                     bool expected) {
  DoTestFull(context, line, regexp, text, expected);
  if (expected) {
    DoTestAnywhere(context, line, regexp, text, expected);
    MatchOffsets expected_match = {0, static_cast<int>(text.size())};
    DoTestFirst(context, line, regexp, text, expected, expected_match);
    DoTestAll(context, line, regexp, text, expected, {expected_match});
  }
}


static void TestFirst(TestContext* context, unsigned line,
                      const char* regexp, const string& text,
                      bool expected,
                      MatchOffsets expected_match,
                      bool bound) {
  DoTestFirst(context, line, regexp, text,
              expected, expected_match);
  DoTestAnywhere(context, line, regexp, text, expected);
  DoTestAll(context, line, regexp, text, expected, {expected_match}, true);
  if (!bound) {
    string extended_text(text);
    extended_text.append(x100(" "));

    DoTestAnywhere(context, line, regexp, extended_text, expected);
    DoTestFirst(context, line, regexp, extended_text, expected, expected_match);
    DoTestAll(context, line, regexp, text, expected, {expected_match}, true);

    extended_text.insert(0, 100, '_');
    expected_match.start += 100;
    expected_match.end += 100;

    DoTestAnywhere(context, line, regexp, extended_text, expected);
    DoTestFirst(context, line, regexp, extended_text, expected, expected_match);
    DoTestAll(context, line, regexp, extended_text, expected, {expected_match}, true);
  }
}


static void TestAllHelper(TestContext* context, unsigned line,
                          const char* regexp, const string& text,
                          const std::vector<MatchOffsets>& expected_matches_) {
  std::vector<MatchOffsets> expected_matches(expected_matches_);
  unsigned expected = expected_matches_.size();
  DoTestAll(context, line, regexp, text, expected, expected_matches);

  if (expected_matches.size() == 1) {
    MatchOffsets expected_match = expected_matches[0];
    if ((expected_match.start == 0) &&
        (expected_match.end == static_cast<int>(text.size()))) {
      DoTestFull(context, line, regexp, text, 1);
    }
  }

  DoTestAnywhere(context, line, regexp, text, expected);
  if (expected > 0) {
    DoTestFirst(context, line, regexp, text, expected, expected_matches[0]);
  } else {
    DoTestFirst(context, line, regexp, text, expected, {-1, -1});
  }

  unsigned n_matches = expected_matches.size();
  size_t local_offset = 0;
  for (unsigned i = 0; i < n_matches; i++) {
    MatchOffsets previous_match = expected_matches[0];
    expected_matches.erase(expected_matches.begin());
    for (std::vector<MatchOffsets>::iterator it = expected_matches.begin();
         it != expected_matches.end();
         it++) {
      it->start -= previous_match.end;
      it->end -= previous_match.end;
    }
    local_offset += previous_match.end;

    MatchOffsets local_match = expected_matches[0];
    string local_text(text, local_offset);
    unsigned local_expected = expected - i - 1;

    DoTestAll(context, line, regexp, local_text, local_expected, expected_matches);
    DoTestAnywhere(context, line, regexp, local_text, local_expected);
    DoTestFirst(context, line, regexp, local_text, local_expected, local_match);
  }
}

static void TestAll(
    TestContext* context, unsigned line,
    const char* regexp, const string& text,
    const std::vector<MatchOffsets>& expected_matches_,
    bool bound) {
  std::vector<MatchOffsets> expected_matches(expected_matches_);
  TestAllHelper(context, line,
                regexp, text,
                expected_matches);

  string extended_text(text);
  extended_text.append(x100(" "));

  if (!bound) {
    TestAllHelper(context, line,
                  regexp, extended_text,
                  expected_matches);

    extended_text.insert(0, 100, '_');
    for (vector<MatchOffsets>::iterator it = expected_matches.begin();
         it != expected_matches.end();
         it++) {
      it->start += 100;
      it->end += 100;
    }

    TestAllHelper(context, line,
                  regexp, extended_text,
                  expected_matches);

  }
}

}  // namespace regit


int main(int argc, char *argv[]) {
  memset(&arguments, 0, sizeof(arguments));
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  return regit::RunTests(&arguments);
}

#undef TEST
