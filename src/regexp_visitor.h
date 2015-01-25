#ifndef REGIT_REGEXP_VISITOR_H_
#define REGIT_REGEXP_VISITOR_H_

#include "regexp.h"

namespace regit {
namespace internal {


class RegexpVisitor {
 public:
  RegexpVisitor() {}
  virtual ~RegexpVisitor() {}

  void Visit(const Regexp* regexp) { regexp->Accept(this); }

  // The default visitor.
  virtual void VisitRegexp(const Regexp* regexp) { UNUSED(regexp); }

  // Specialised visitors.
#define DECLARE_REGEXP_VISITORS(RegexpType)                                    \
  virtual void Visit##RegexpType(const RegexpType* r) { VisitRegexp(r); }
  LIST_REAL_REGEXP_TYPES(DECLARE_REGEXP_VISITORS)
#undef DECLARE_REGEXP_VISITORS

 private:
  DISALLOW_COPY_AND_ASSIGN(RegexpVisitor);
};


} }  // namespace regit::internal

#endif  // REGIT_REGEXP_VISITOR_H_

