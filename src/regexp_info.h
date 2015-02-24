#ifndef REGIT_REGEXP_INFO_H_
#define REGIT_REGEXP_INFO_H_

#include "automaton.h"
#include "regexp.h"

namespace regit {
namespace internal {

class RegexpInfo {
 public:
  RegexpInfo() : regexp_(nullptr) {}
  ~RegexpInfo() {
    delete regexp_;
  }

  const Regexp* regexp() const { return regexp_; }
  void set_regexp(Regexp* regexp) { regexp_ = regexp; }

  const Automaton* automaton() const { return automaton_; }
  void set_automaton(Automaton* automaton) { automaton_ = automaton; }


 private:
  Regexp* regexp_;
  Automaton* automaton_;
};


} }  // namespace regit::internal

#endif  // REGIT_REGEXP_INFO_H_
