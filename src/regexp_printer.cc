#include "regexp_printer.h"

namespace regit {
namespace internal {

void RegexpPrinter::VisitMultipleChar(const MultipleChar* mc) {
  if (parameters_ & kShortName) {
    Indent(cout) << mc->Chars();
  } else {
    Indent(cout) << "MultipleChar {" << mc->Chars() << "}";
  }
  if (parameters_ & kPrintNewLine) cout << "\n";
}


void RegexpPrinter::VisitPeriod(const Period* period) {
  UNUSED(period);
  if (parameters_ & kShortName) {
    Indent(cout) << ".";
  } else {
    Indent(cout) << "Period(" << (period->posix() ? "posix" : "standard") << ")";
  }
  if (parameters_ & kPrintNewLine) cout << "\n";
}


void RegexpPrinter::VisitEpsilon(const Epsilon*) {
  if (parameters_ & kShortName) {
    Indent(cout) << "ε";
  } else {
    Indent(cout) << "epsilon transition";
  }
  if (parameters_ & kPrintNewLine) cout << "\n";
}


void RegexpPrinter::VisitConcatenation(const Concatenation* concatenation) {
  Indent(cout) << "Concatenation {\n";
  {
    IndentationScope indent(this);
    for (Regexp* re : *concatenation->sub_regexps()) {
      Visit(re);
    }
  }
  Indent(cout) << "}";
  if (parameters_ & kPrintNewLine) cout << "\n";
}


void RegexpPrinter::VisitAlternation(const Alternation* alternation) {
  Indent(cout) << "Alternation {\n";
  {
    IndentationScope indent(this);
    for (Regexp* re : *alternation->sub_regexps()) {
      Visit(re);
    }
  }
  Indent(cout) << "}";
  if (parameters_ & kPrintNewLine) cout << "\n";
}

} }  // namespace regit::internal
