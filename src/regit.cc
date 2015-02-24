#include "automaton.h"
#include "parser.h"
#include "regexp_info.h"
#include "regit.h"

namespace regit {

const Options regit_default_options;

Regit::Regit(const char* regexp) :
    regexp_(regexp), regexp_size_(strlen(regexp)),
    status_(kSuccess),
    rinfo_(new internal::RegexpInfo()) {}

Regit::Regit(const string& regexp) :
    regexp_(regexp.c_str()), regexp_size_(regexp.size()),
    status_(kSuccess),
    rinfo_(new internal::RegexpInfo()) {}

Regit::~Regit() {
  delete rinfo_;
}

void Regit::Compile(const Options* options) {
  internal::Parser parser(options);
  internal::Regexp* re = parser.Parse(regexp_, regexp_size_);
  if (re == nullptr) {
    ASSERT(parser.status() != kSuccess);
    status_ = parser.status();
    return;
  }
  rinfo_->set_regexp(re);
  internal::Automaton* automaton = new internal::Automaton(re);
  if (automaton == nullptr) {
    status_ = kOutOfMemory;
    return;
  }
  if (automaton->status() != kSuccess) {
    return;
  }
  rinfo_->set_automaton(automaton);
}

}  // namespace regit
