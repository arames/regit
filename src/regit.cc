#include "parser.h"
#include "regit.h"

namespace regit {

const Options regit_default_options;

Regit::Regit(const char* regexp) :
    regexp_(regexp), regexp_size_(strlen(regexp)),
    rinfo_(new internal::RegexpInfo()) {}

Regit::Regit(const string& regexp) :
    regexp_(regexp.c_str()), regexp_size_(regexp.size()),
    rinfo_(new internal::RegexpInfo()) {}

Regit::~Regit() {
  delete rinfo_;
}

void Regit::Compile(const Options* options) {
  internal::Parser parser(options);
  internal::Regexp* re = parser.Parse(regexp_, regexp_size_);
  rinfo_->set_regexp(re);
}

}  // namespace regit
