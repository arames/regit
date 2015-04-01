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


bool Regit::MatchFull(const string& text) {
  return MatchFull(text.c_str(), text.size());
}


bool Regit::MatchFull(const char* text, size_t text_size) {
  if (!rinfo_->compiled()) {
    Compile();
  }
  if (status_ != kSuccess) {
    return false;
  }
  internal::Simulation simulation(rinfo_->automaton());
  return simulation.MatchFull(text, text_size);
}


bool Regit::MatchAnywhere(Match* match, const string& text) {
  return MatchAnywhere(match, text.c_str(), text.size());
}


bool Regit::MatchAnywhere(Match* match, const char* text, size_t text_size) {
  if (!rinfo_->compiled()) {
    Compile();
  }
  if (status_ != kSuccess) {
    return false;
  }
  internal::Simulation simulation(rinfo_->automaton());
  return simulation.MatchAnywhere(match, text, text_size);
}


bool Regit::MatchFirst(Match* match, const string& text) {
  return MatchFirst(match, text.c_str(), text.size());
}


bool Regit::MatchFirst(Match* match, const char* text, size_t text_size) {
  if (!rinfo_->compiled()) {
    Compile();
  }
  if (status_ != kSuccess) {
    return false;
  }
  internal::Simulation simulation(rinfo_->automaton());
  return simulation.MatchFirst(match, text, text_size);
}


bool Regit::MatchAll(vector<Match>* matches, const string& text) {
  return MatchAll(matches, text.c_str(), text.size());
}


bool Regit::MatchAll(vector<Match>* matches, const char* text, size_t text_size) {
  if (!rinfo_->compiled()) {
    Compile();
  }
  if (status_ != kSuccess) {
    return false;
  }
  internal::Simulation simulation(rinfo_->automaton());
  return simulation.MatchAll(matches, text, text_size);
}


}  // namespace regit
