#include "automaton.h"
#include "regexp_printer.h"


namespace regit {
namespace internal {


void Automaton::BuildFrom(Regexp* regexp) {
  RegexpIndexer indexer(this);
  entry_state_ = NewState();
  last_state_ = entry_state_;
  exit_state_ = entry_state_;
  indexer.Visit(regexp);

  if (FLAG_print_automaton) {
    Print();
  }
}

State* Automaton::NewState() {
  State* state = new State();
  if (state == nullptr) {
    status_ = kOutOfMemory;
  }
  state->set_index(states_.size());
  last_state_ = state;
  states_.push_back(state);
  return state;
}


void Automaton::Print() const {
  RegexpPrinter printer(RegexpPrinter::kShortName);
  cout << "digraph regexp {\n"
      << "  rankdir=\"LR\";  // We prefer an horizontal graph.\n"
      << "  // Entry state: " << entry_state_->index() << "\n"
      << "  // Exit state: " << exit_state_->index() << "\n";
  for (State* state : states_) {
    for (const Regexp* regexp : *state->from()) {
      cout << "  " << state->index() << " -> " << regexp->exit()->index()  << " [label=\"";
      printer.Visit(regexp);
      cout << "\"];\n";
    }
  }
  cout << "}\n";
}


void RegexpIndexer::VisitRegexp(Regexp* regexp,
                                State* entry_state,
                                State* exit_state) {
  State* entry = (exit_state == nullptr) ? automaton_->last_state_ : entry_state;
  State* exit = (exit_state == nullptr) ? automaton_->NewState() : exit_state;
  regexp->set_entry(entry);
  regexp->set_exit(exit);
  automaton_->exit_state_ = exit;
  entry->from_.push_back(regexp);
  exit->to_.push_back(regexp);
}


void RegexpIndexer::VisitAlternation(Alternation* alternation,
                                     State* entry_state,
                                     State* exit_state) {
  State* entry = (entry_state != nullptr) ? entry_state
                                          : automaton_->last_state_;
  State* exit = (exit_state != nullptr) ? exit_state
                                        : automaton_->NewState();
  for (Regexp* regexp : *alternation->sub_regexps()) {
    Visit(regexp, entry, exit);
  }
}


void RegexpIndexer::VisitConcatenation(Concatenation* concatenation,
                                       State* entry_state,
                                       State* exit_state) {
  const vector<Regexp*>* sub_regexps = concatenation->sub_regexps();
  ASSERT(!sub_regexps->empty());
  State* previous_exit = (entry_state != nullptr) ? entry_state
                                                  : automaton_->last_state_;
  State* exit;
  for (vector<Regexp*>::const_iterator it = sub_regexps->cbegin();
       it < sub_regexps->cend() - 1;
       it++) {
    exit = automaton_->NewState();
    Visit(*it, previous_exit, exit);
    previous_exit = exit;
  }
  exit = (exit_state != nullptr) ?  exit_state : automaton_->NewState();
  Visit(sub_regexps->back(), previous_exit, exit);
}


} }  // namespace regit::internal
