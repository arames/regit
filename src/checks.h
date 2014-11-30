#ifndef REGIT_CHECKS_H
#define REGIT_CHECKS_H

#include <assert.h>
#include <stdarg.h>

void regit_fatal(const char* file, int line, const char* format, ...);

#define STATIC_ASSERT(condition) static_assert((condition), "static_assert")
#define ALWAYS_ASSERT(condition) assert((condition))
#define FATAL(s) regit_fatal(__FILE__, __LINE__, "FATAL: %s\n", s)

#ifdef DEBUG
#define ASSERT(condition) assert((condition))
#define UNREACHABLE() regit_fatal(__FILE__, __LINE__, "UNREACHABLE\n")
#define UNIMPLEMENTED() regit_fatal(__FILE__, __LINE__, "UNIMPLEMENTED\n")
#else
#define ASSERT(condition)
#define UNREACHABLE() regit_fatal("", 0, "UNREACHABLE\n")
#define UNIMPLEMENTED() regit_fatal("", 0, "UNIMPLEMENTED\n")
#endif

#endif  // REGIT_CHECKS_H
