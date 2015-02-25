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
  cout << "digraph regexp {\n";
  cout << "  rankdir=\"LR\";  // We prefer an horizontal graph.\n";
  PrintInfo();
  for (State* state : states_) {
    for (const Regexp* regexp : *state->from()) {
      cout << "  " << state->index() << " -> " << regexp->exit()->index()
          << " [label=\"";
      printer.Visit(regexp);
      cout << "\"];\n";
    }
  }
  cout << "}\n";
}


void Automaton::PrintInfo() const {
  cout << "  // Number of states: " << NStates() << "\n"
      << "  // Entry state: " << entry_state()->index() << "\n"
      << "  // Exit state: " << exit_state()->index() << "\n"
      << "  // Max transition match length: " << max_transition_match_length()
      << "\n";
}


void RegexpIndexer::VisitRegexp(Regexp* regexp,
                                State* entry_state,
                                State* exit_state) {
  State* entry = (exit_state == nullptr) ? automaton_->last_state_
                                         : entry_state;
  State* exit = (exit_state == nullptr) ? automaton_->NewState()
                                        : exit_state;
  regexp->set_entry(entry);
  regexp->set_exit(exit);
  automaton_->exit_state_ = exit;
  entry->from_.push_back(regexp);
  exit->to_.push_back(regexp);

  automaton_->UpdateMaxTransitionMatchLength(regexp->MatchLength());
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


bool Simulation::MatchFull(const char* text, size_t text_size) {
  text_ = text;
  text_end_ = text + text_size;
  current_pos_ = text_;
  current_tick_ = 0;
  SetState(text, automaton_->entry_state(), 0);
  if (FLAG_trace_matching) { Print(); }
  while (remaining_text_size() != 0) {
    for (const State* state : *automaton_->states()) {
      pos_t state_pos = GetState(state, 0);
      if (state_pos != kInvalidPos) {
        for (const Regexp* regexp : *state->from()) {
          int chars_matched = regexp->Match(current_pos_);
          if (chars_matched != -1) {
            SetState(state_pos, regexp->exit(), chars_matched);
          }
        }
      }
    }
    InvalidateTick(0);
    Advance(1);
    if (FLAG_trace_matching) { Print(); }
  }
  return GetState(automaton_->exit_state(), 0) != kInvalidPos;
}


void Simulation::Print(int i) const {
  RegexpPrinter printer(RegexpPrinter::kShortName);

#define ACTIVE_STYLE   "style=bold,color=blue"
#define INACTIVE_STYLE "style=\"\",color=\"\""

  int current_index = CurrentIndex();
  cout << "digraph regexp_" << current_index << "_" << i << " {\n"
      << "  label=\"index " << current_index << " tick " << i << "\\n"
      <<           "position: " << current_pos_ + i << "\";\n"
      << "  labelloc=t;\n"
      << "  rankdir=\"LR\";  // We prefer an horizontal graph.\n";
  automaton_->PrintInfo();

  cout << "  // Active states.\n"
      << "  node [" ACTIVE_STYLE "];\n";
  for (const State* state : *automaton_->states()) {
      if (GetState(state, i) != kInvalidPos) {
        cout << "  " << state->index() << ";\n";
      }
  }
  cout << "  // Transitions.\n";
  cout << "  node [" INACTIVE_STYLE "];\n";
  for (const State* state : *automaton_->states()) {
    bool active_state = GetState(state, i) != kInvalidPos;
    for (const Regexp* regexp : *state->from()) {
      cout << "  " << state->index() << " -> " << regexp->exit()->index()
          << " [label=";
      printer.Visit(regexp);
      cout << "\"";
      if (active_state && (i == 0) && (-1 != regexp->Match(current_pos_))) {
        cout << "," ACTIVE_STYLE;
      }
      cout << "];\n";
    }
  }
  cout << "}\n";
}


} }  // namespace regit::internal
