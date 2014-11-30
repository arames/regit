#include "checks.h"
#include <stdio.h>
#include <stdlib.h>

void regit_fatal(const char* file, int line, const char* format, ...) {
  printf("ERROR: in %s, line %d: ", file, line);
  va_list args;
  va_start(args, format);
  printf(format, args);
  va_end(args);
  abort();
}
