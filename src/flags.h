#ifndef REGIT_FLAGS_H_
#define REGIT_FLAGS_H_

// This is a centralised definition of the flags used by regit.
// This allows to easily use macros to operate on all the flags at once.
// Note that defined macros are required to take 4 arguments, but can ignore
// some of them. See for example the DECLARE_FLAG macro.

// The declarations follow the format:

#define REGIT_PRINT_FLAGS_LIST(M)                                              \
M( print_re_tree         , false   , false ,                                   \
   "After parsing a regexp, print the regexp tree." )                          \
M( print_automaton       , false   , false ,                                   \
   "After building the automaton for a regexp, print it." )                    \
M( trace_matching        , false   , false ,                                   \
   "Trace the matching process.")

#define REGIT_FLAGS_LIST(M)                                                    \
M( parser_opt            , true    , true  ,                                   \
   "Enable parser optimisations." )                                            \
REGIT_PRINT_FLAGS_LIST(M)

// Declare all the flags.
#ifdef MODIFIABLE_FLAGS
#define DECLARE_FLAG(name, r, d, desc) extern bool FLAG_##name;
#elif defined(DEBUG)
#define DECLARE_FLAG(name, r, d, desc) static constexpr bool FLAG_##name = d;
#else
#define DECLARE_FLAG(name, r, d, desc) static constexpr bool FLAG_##name = r;
#endif

REGIT_FLAGS_LIST(DECLARE_FLAG)

#undef DECLARE_FLAG

#ifdef MODIFIABLE_FLAGS
#define SET_FLAG(name, val) FLAG_##name = val
#else
#define SET_FLAG(name, val)
#endif

#define GET_FLAG(name) FLAG_##name

#endif  // REGIT_FLAGS_H_
