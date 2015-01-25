#ifndef REGIT_REGEXP_PRINTER_H_
#define REGIT_REGEXP_PRINTER_H_

#include <ostream>

#include "regexp.h"
#include "regexp_visitor.h"


namespace regit {
namespace internal {

class RegexpPrinter : public RegexpVisitor {
 public:
  enum Parameters {
    kPrintNewLine = 1 << 0,
    kShortName    = 1 << 1
  };
  RegexpPrinter(Parameters parameters = kPrintNewLine)
      : indentation_level_(0), parameters_(parameters) {}

#define DECLARE_REGEXP_VISITORS(RegexpType) \
  void Visit##RegexpType(const RegexpType* r) OVERRIDE;
  LIST_REAL_REGEXP_TYPES(DECLARE_REGEXP_VISITORS)
#undef DECLARE_REGEXP_VISITORS

  std::ostream& Indent(std::ostream& stream) {
    return stream << string(indentation_level_, ' ');
  }

  class IndentationScope {
   public:
    static constexpr int kDefaultIndent = 2;
    IndentationScope(RegexpPrinter* printer, int indent = kDefaultIndent)
        : printer_(printer) {
          previous_indentation_level_ = printer->indentation_level_;
          printer->indentation_level_ += indent;
        }
    ~IndentationScope() {
      printer_->indentation_level_ = previous_indentation_level_;
    }
    RegexpPrinter* printer_;
    int previous_indentation_level_;
  };

 private:
  int indentation_level_;
  Parameters parameters_;

  friend class IndentationScope;
};

} }  // namespace regit::internal

#endif  // REGIT_REGEXP_PRINTER_H_

