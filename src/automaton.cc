#include "automaton.h"
#include "regexp_printer.h"


namespace regit {
namespace internal {

void Automaton::BuildFrom(Regexp* regexp) {
  RegexpIndexer indexer(this);
  indexer.Visit(regexp);

  if (FLAG_print_automaton) {
    Print();
  }
}

void Automaton::Print() const {
  RegexpPrinter printer(RegexpPrinter::kShortName);
  cout << "digraph regexp {\n"
      << "  rankdir=\"LR\";  // We prefer an horizontal graph.\n"
      << "  // Entry state: " << entry_state_ << "\n"
      << "  // Exit state: " << exit_state_ << "\n";
  for (std::pair<state_t, struct Transition> pair : transitions_from_) {
    Transition t = pair.second;
    cout << "  " << t.from << " -> " << t.to << " [label=\"";
    printer.Visit(t.regexp);
    cout << "\"];\n";
  }
  cout << "}\n";
}

void RegexpIndexer::VisitRegexp(const Regexp* regexp,
                                int entry_state,
                                int exit_state) {
  struct Automaton::Transition t;
  t.regexp = regexp;
  t.from = (exit_state == kNoSpecifiedState) ? automaton_->last_state_
                                             : entry_state;
  t.to = (exit_state == kNoSpecifiedState) ? ++automaton_->last_state_
                                           : exit_state;
  automaton_->exit_state_ = t.to;

  automaton_->transitions_[regexp] = t;
  automaton_->transitions_from_.insert(
      pair<state_t, struct Automaton::Transition>(t.from, t));
  automaton_->transitions_to_.insert(
      pair<state_t, struct Automaton::Transition>(t.to, t));
}

void RegexpIndexer::VisitAlternation(const Alternation* alternation,
                                     int entry_state,
                                     int exit_state) {
  int entry = (entry_state != kNoSpecifiedState) ? entry_state
                                                 : automaton_->last_state_;
  int exit = (exit_state != kNoSpecifiedState) ? exit_state
                                               : ++automaton_->last_state_;
  for (Regexp* regexp : *alternation->sub_regexps()) {
    Visit(regexp, entry, exit);
  }
}

void RegexpIndexer::VisitConcatenation(const Concatenation* concatenation,
                                       int entry_state,
                                       int exit_state) {
  const vector<Regexp*>* sub_regexps = concatenation->sub_regexps();
  ASSERT(!sub_regexps->empty());
  int previous_exit = (entry_state != kNoSpecifiedState) ? entry_state
                                                         : automaton_->last_state_;
  int exit;
  for (vector<Regexp*>::const_iterator it = sub_regexps->cbegin();
       it < sub_regexps->cend() - 1;
       it++) {
    exit = ++automaton_->last_state_;
    Visit(*it, previous_exit, exit);
    previous_exit = exit;
  }
  exit = (exit_state != kNoSpecifiedState) ?  exit_state : ++automaton_->last_state_;
  Visit(sub_regexps->back(), previous_exit, exit);
}

} }  // namespace regit::internal
