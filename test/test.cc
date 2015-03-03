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


static int test_id = 0;
// Every call to this function increments the global counter `test_id`, even
// when the test is skipped.
enum TestStatus {
  TEST_SKIPPED = 0,
  TEST_PASSED = 1,
  TEST_FAILED = -1
};


static TestStatus TestFull(const struct arguments *arguments,
                           const char* regexp, const string& text,
                           bool expected,
                           int line) {
  ++test_id;
  if (!ShouldTest(arguments, line, test_id)) {
    return TEST_SKIPPED;
  }

  if (arguments->verbose) {
    PrintTest(line, test_id);
  }

  bool exception_occurred = false;
  bool res = 0;

  try {
    Regit re(regexp);
    res = re.MatchFull(text);
  } catch (int e) {
    exception_occurred = true;
  }

  if (exception_occurred || res != expected) {
    printf("FAILED line %d test_id %d\n", line, test_id);
    printf("regexp:\n%s\n", regexp);
    printf("text:\n%s\n", text.c_str());
    printf("expected: %d  found:  %d", expected, res);
  }

  TestStatus status =
      (exception_occurred || res != expected) ? TEST_FAILED : TEST_PASSED;
  assert(!arguments->break_on_fail && (status == TEST_PASSED));
  return status;
}


int RunTest(const struct arguments *arguments) {
  TestStatus local_rc;
  int count_passed = 0;
  int count_failed = 0;
  int count_skipped = 0;

#define UPDATE_RESULTS(local_rc)                                               \
  count_passed += (local_rc == TEST_PASSED);                                   \
  count_failed += (local_rc == TEST_FAILED);                                   \
  count_skipped += (local_rc == TEST_SKIPPED)

#define TEST_Full(expected, re, text)                                          \
  local_rc = TestFull(arguments, re, string(text), expected, __LINE__);        \
  UPDATE_RESULTS(local_rc)

  TEST_Full(1, "x", "x");
  TEST_Full(0, "x", "y");
  TEST_Full(0, "x", "xxxxxx");
  TEST_Full(1, "abcdefghij", "abcdefghij");
  TEST_Full(0, "abcdefghij", "abcdefghij_klmnop");

  // More characters than can be hold within one MultipleChar.
  TEST_Full(1, x10("abcdefghij"), x10("abcdefghij"));
  TEST_Full(0, x10("abcdefghij"), x10("abcdefghij") "X");
  TEST_Full(0, x10("abcdefghij"), "X" x10("abcdefghij"));
  TEST_Full(1, x100("abcdefghij"), x100("abcdefghij"));
  TEST_Full(0, x100("abcdefghij"), x100("abcdefghij") "X");
  TEST_Full(0, x100("abcdefghij"), "X" x100("abcdefghij"));

  // Period.
  TEST_Full(1, ".", "x");
  TEST_Full(0, ".", "\n");
  TEST_Full(0, ".", "\r");
  TEST_Full(1, "abcde.ghij", "abcdefghij");
  TEST_Full(0, "abcdefghi.", "abcdefghijklmn");
  TEST_Full(0, "a.b", "a\nb");
  TEST_Full(0, "a.b", "a\rb");
  TEST_Full(1, "...", "abc");
  TEST_Full(0, "...", "ab");
  TEST_Full(0, "..", "abc");
  TEST_Full(1, x100("abcde.ghij"), x100("abcde.ghij"));
  TEST_Full(0, x100("abcde.ghij"), x100("abcde.ghij") "X");
  TEST_Full(0, x100("abcde.ghij"), "X" x100("abcde.ghij"));

  // Alternation.
  TEST_Full(1, "abcd|efgh", "abcd");
  TEST_Full(1, "abcd|efgh", "efgh");
  TEST_Full(1, "abcd|efgh|ijkl", "abcd");
  TEST_Full(1, "abcd|efgh|ijkl", "efgh");
  TEST_Full(1, "abcd|efgh|ijkl", "ijkl");
  TEST_Full(0, "abcd|efgh|ijkl", "_efgh___");
  TEST_Full(0, "abcd|efgh|ijkl", "abcdefghijkl");
  TEST_Full(0, "abcd|efgh|ijkl", "abcd|efgh|ijkl");

  TEST_Full(1, "..(abcX|abcd)..", "..abcd..");
  TEST_Full(1, "..(abcd|abcX)..", "..abcd..");

  if (count_failed) {
      printf("passed: %d\tfailed: %d\tskipped: %d(total: %d)\n",
             count_passed,
             count_failed,
             count_skipped,
             count_failed + count_passed + count_skipped);
  } else {
    printf("success\n");
  }
  return count_failed;
}


}  // namespace regit


int main(int argc, char *argv[]) {
  memset(&arguments, 0, sizeof(arguments));
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  return regit::RunTest(&arguments);
}

#undef TEST
