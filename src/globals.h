#ifndef REGIT_GLOBALS_H_
#define REGIT_GLOBALS_H_

#include <stdint.h>

#include "checks.h"
#include "flags.h"

namespace regit {
namespace internal {

#define DISALLOW_COPY_AND_ASSIGN(Type) \
  Type(const Type&);                   \
  void operator=(const Type&)

// Use this to avoid unused variable warnings.
template <typename T> void UNUSED(T) {}

} }  // namespace regit::internal

#endif  // REGIT_GLOBALS_H_

