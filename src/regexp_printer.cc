#include "regexp_printer.h"

namespace regit {
namespace internal {

void RegexpPrinter::VisitMultipleChar(const MultipleChar* mc) {
  Indent(cout) << "MultipleChar {" << mc->chars() << "}" << endl;
}


void RegexpPrinter::VisitPeriod(const Period* period) {
  UNUSED(period);
  Indent(cout) << "Period("
      << (period->posix() ? "posix" : "standard") << ")"<< endl;
}


void RegexpPrinter::VisitEpsilon(const Epsilon*) {
  Indent(cout) << "epsilon transition" << endl;
}


void RegexpPrinter::VisitConcatenation(const Concatenation* concatenation) {
  Indent(cout) << "Concatenation {" << endl;
  {
    IndentationScope indent(this);
    for (Regexp* re : *concatenation->sub_regexps()) {
      Visit(re);
    }
  }
  Indent(cout) << "}" << endl;
}


void RegexpPrinter::VisitAlternation(const Alternation* alternation) {
  Indent(cout) << "Alternation {" << endl;
  {
    IndentationScope indent(this);
    for (Regexp* re : *alternation->sub_regexps()) {
      Visit(re);
    }
  }
  Indent(cout) << "}" << endl;
}

} }  // namespace regit::internal
