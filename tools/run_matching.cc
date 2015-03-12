#include <argp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "regit.h"
#include "flags.h"

#ifndef MOD_FLAGS
#error "compinfo requires the regit flags to be modifiable."
#endif

// Start the enum from the latest argp key used.
enum regit_flags_option_keys {
  // Hope it does not collide with other keys.
  base_regit_flag_key = 0x7BAD,
#define ENUM_KEYS(flag_name, r, d, desc) flag_name##_key,
  REGIT_FLAGS_LIST(ENUM_KEYS)
#undef ENUM_KEYS
  after_last_regit_key,
  first_regit_flag_key = base_regit_flag_key + 1
};
#define REGIT_FLAG_OFFSET(flag_name) (flag_name##_key - first_regit_flag_key)

struct arguments {
  const char *regexp;
  const char *text;
  regit::MatchType match_type;
  int  regit_flags;
};

struct argp_option options[] =
{
#define FLAG_OPTION(flag_name, r, d, desc)                                     \
  {#flag_name , flag_name##_key , FLAG_##flag_name ? "1" : "0",                \
    OPTION_ARG_OPTIONAL , desc "\n0 to disable, 1 to enable.", 1},
  REGIT_FLAGS_LIST(FLAG_OPTION)
#undef FLAG_OPTION
  {"match_type" , 'm' , ""  , OPTION_ARG_OPTIONAL ,
    "Type of matching to perform (full or first).", 2},
  {"print_all" , 'p' , ""  , OPTION_ARG_OPTIONAL ,
    "Enable or disable all --print* options.", 3},
  {nullptr, 0, nullptr, 0, nullptr, 0}
};

char args_doc[] = "regexp text";
char doc[] =
"Try to match \"regexp\" in \"text\".";
const char *argp_program_bug_address = "<alexandre@uop.re>";
error_t parse_opt(int key, char *arg, struct argp_state *state);
struct argp argp = {options, parse_opt, args_doc, doc, nullptr, nullptr, nullptr};


error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = reinterpret_cast<struct arguments*>(state->input);
  switch (key) {
    case 'p': {
      unsigned v = (arg != nullptr) ? stol(arg) : 1;
      assert(v == 0 || v == 1);
#define SET_REGIT_FLAG(flag_name, r, d, desc)                                  \
      if (v == 1) {                                                            \
        arguments->regit_flags |= 1 << REGIT_FLAG_OFFSET(flag_name);           \
      } else {                                                                 \
        arguments->regit_flags &= ~(1 << REGIT_FLAG_OFFSET(flag_name));        \
      }
      REGIT_PRINT_FLAGS_LIST(SET_REGIT_FLAG)
#undef SET_REGIT_FLAG
      break;
    }

#define FLAG_CASE(flag_name, r, d, desc)                                       \
    case flag_name##_key: {                                                    \
      unsigned v = (arg != nullptr) ? stol(arg) : 1;                           \
      assert(v == 0 || v == 1);                                                \
      if (v == 1) {                                                            \
        arguments->regit_flags |= 1 << REGIT_FLAG_OFFSET(flag_name);           \
      } else {                                                                 \
        arguments->regit_flags &= ~(1 << REGIT_FLAG_OFFSET(flag_name));        \
      }                                                                        \
      break;                                                                   \
    }
    REGIT_FLAGS_LIST(FLAG_CASE)
#undef FLAG_CASE

    case 'm': {
      if (arg == nullptr) {
        argp_usage(state);
      }
      if (strcmp(arg, "full") == 0) {
        arguments->match_type = regit::kFull;
      } else if (strcmp(arg, "first") == 0) {
        arguments->match_type = regit::kFirst;
      } else {
        argp_usage(state);
      }
      break;
    }

    case ARGP_KEY_ARG:
      if (state->arg_num >= 3) {
        argp_usage(state);
      }
      if (state->arg_num == 0) {
        arguments->regexp = arg;
        break;
      }
      if (state->arg_num == 1) {
        arguments->text = arg;
        break;
      }
      break;
    case ARGP_KEY_END:
      if (state->arg_num < 1) {
        argp_usage(state);
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


void handle_arguments(struct arguments *arguments,
                      struct argp *argp,
                      int argc,
                      char *argv[]) {
  arguments->regexp = nullptr;
  arguments->text = nullptr;
  arguments->match_type = regit::kFull;

#define SET_FLAG_DEFAULT(flag_name, r, d, desc)                                \
  arguments->regit_flags |= FLAG_##flag_name << REGIT_FLAG_OFFSET(flag_name);
  arguments->regit_flags = 0;
  REGIT_FLAGS_LIST(SET_FLAG_DEFAULT)
#undef SET_FLAG_DEFAULT

  argp_parse(argp, argc, argv, 0, 0, arguments);

  if (arguments->regexp == nullptr || arguments->regexp[0] == '\0') {
    printf("ERROR: Cannot test an empty regular expression.\n");
    argp_usage(nullptr);
  }

  // Set regit flags according to the arguments.
#define MAYBE_SET_REGIT_FLAG(flag_name, r, d, desc)                            \
  SET_FLAG(flag_name,                                                          \
           arguments->regit_flags & (1 << REGIT_FLAG_OFFSET(flag_name)));
  REGIT_FLAGS_LIST(MAYBE_SET_REGIT_FLAG)
#undef MAYBE_SET_REGIT_FLAG
}


int main(int argc, char* argv[]) {
  struct arguments arguments;
  handle_arguments(&arguments, &argp, argc, argv);

  regit::Regit re(arguments.regexp);

  switch (arguments.match_type) {
    case regit::kFull:
      re.MatchFull(arguments.text);
      break;
    case regit::kFirst:
      regit::Match match;
      re.MatchFirst(&match, arguments.text);
      break;
    default:
      argp_usage(nullptr);
  }


  return EXIT_SUCCESS;
}