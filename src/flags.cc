#include "flags.h"

#ifdef MODIFIABLE_FLAGS
#ifdef DEBUG
#define DEFINE_FLAG(name, r, d, desc) bool FLAG_##name = d;
#else
#define DEFINE_FLAG(name, r, d, desc) bool FLAG_##name = r;
#endif
REGIT_FLAGS_LIST(DEFINE_FLAG)
#undef DEFINE_FLAG
#endif
