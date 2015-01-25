#include <map>
#include <string.h>

#include "regexp.h"
#include "regexp_printer.h"
#include "regexp_visitor.h"


namespace regit {
namespace internal {

void Regexp::Print() const {
  RegexpPrinter printer;
  printer.Visit(this);
}

#define DEFINE_ACCEPT(Name)                                                    \
void Name::Accept(RegexpVisitor* visitor) const {                              \
  visitor->Visit##Name(this);                                                  \
}
LIST_REAL_REGEXP_TYPES(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT

} }  // namespace regit::internal

