#include <map>
#include <string.h>

#include "regexp.h"
#include "regexp_printer.h"


namespace regit {
namespace internal {

void Regexp::Print() const {
  RegexpPrinter printer;
  printer.Visit(this);
}


} }  // namespace regit::internal

