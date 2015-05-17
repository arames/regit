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

  while (remaining_text_size() != 0) {
    for (const State* state : *automaton_->states()) {
      pos_t state_pos = GetState(state, 0);
      if (state_pos != kInvalidPos) {
        for (const Regexp* regexp : *state->from()) {
          int chars_matched = regexp->Match(current_pos_);
          if (chars_matched != -1) {
            UpdateState(state_pos, regexp->exit(), chars_matched);
          }
        }
      }
    }
    if (FLAG_trace_matching) { Print(); }
    InvalidateTick(0);
    Advance(1);
  }

  if (FLAG_trace_matching) { Print(); }
  return GetState(automaton_->exit_state(), 0) != kInvalidPos;
}


bool Simulation::MatchAnywhere(Match* match, const char* text, size_t text_size) {
  text_ = text;
  text_end_ = text + text_size;
  current_pos_ = text_;
  current_tick_ = 0;

  while (remaining_text_size() != 0) {
    SetState(current_pos_, automaton_->entry_state(), 0);
    for (const State* state : *automaton_->states()) {
      pos_t state_pos = GetState(state, 0);
      if (state_pos != kInvalidPos) {
        for (const Regexp* regexp : *state->from()) {
          int chars_matched = regexp->Match(current_pos_);
          if (chars_matched != -1) {
            UpdateState(state_pos, regexp->exit(), chars_matched);
          }
        }
      }
    }
    if (FLAG_trace_matching) { Print(); }
    InvalidateTick(0);
    Advance(1);
    pos_t found_pos = GetState(automaton_->exit_state(), 0);
    if (found_pos != kInvalidPos) {
      match->start = found_pos;
      match->end = current_pos_;
      return true;
    }
  }

  if (FLAG_trace_matching) { Print(); }
  return false;
}


bool Simulation::MatchFirst(Match* match, const char* text, size_t text_size) {
  text_ = text;
  text_end_ = text + text_size;
  current_pos_ = text_;
  current_tick_ = 0;

  bool found_match = false;

  while (remaining_text_size() != 0) {
    if (!found_match) {
      SetState(current_pos_, automaton_->entry_state(), 0);
    }
    for (const State* state : *automaton_->states()) {
      pos_t state_pos = GetState(state, 0);
      if (state_pos != kInvalidPos) {
        for (const Regexp* regexp : *state->from()) {
          int chars_matched = regexp->Match(current_pos_);
          if (chars_matched != -1) {
            UpdateState(state_pos, regexp->exit(), chars_matched);
          }
        }
      }
    }
    if (FLAG_trace_matching) { Print(); }
    InvalidateTick(0);
    Advance(1);
    pos_t found_pos = GetState(automaton_->exit_state(), 0);
    if (found_pos != kInvalidPos) {
      // Any newly found match must be preferable to the previously found match.
      ASSERT(!found_match || (found_pos <= match->start));
      ASSERT(!found_match || (current_pos_ >= match->end));
      if (!found_match) {
        InvalidateStatesAfter(found_pos);
      }
      found_match = true;
      match->start = found_pos;
      match->end = current_pos_;
    }
  }

  if (FLAG_trace_matching) { Print(); }
  return found_match;
}


bool Simulation::MatchAll(vector<Match>* matches, const char* text, size_t text_size) {
  bool found_match;
  bool has_matched = false;
  Match match;
  do {
    found_match = MatchFirst(&match, text, text_size);
    if (found_match) {
      matches->push_back(match);
      has_matched = true;
      text_size -= (match.end - text);
      text = match.end;
    }
  } while (found_match && (text_size != 0));
  return has_matched;
}


void Simulation::InvalidateStatesAfter(pos_t start) {
  for (int tick = 0; tick < n_ticks_; tick++) {
    for (const State* state : *automaton_->states()) {
      if (GetState(state, tick) > start) {
        InvalidateState(state, tick);
      }
    }
  }
}


void Simulation::Print(int tick) const {
  RegexpPrinter printer(RegexpPrinter::kShortName);

#define ACTIVE_STYLE_INITIAL    "style=bold,color=blue"
#define ACTIVE_STYLE_TRANSITION "style=bold,color=orange"
#define ACTIVE_STYLE_FUTURE     ACTIVE_STYLE_TRANSITION
#define INACTIVE_STYLE          "style=\"\",color=\"\""

  int current_index = CurrentIndex();
  cout << "digraph regexp_" << current_index << "_" << tick << " {\n"
      << "  label=\"index " << current_index << " tick " << tick << "\\n"
      <<           "text: " << current_pos_ + tick << "\";\n"
      << "  labelloc=t;\n"
      << "  rankdir=\"LR\";  // We prefer an horizontal graph.\n";
  automaton_->PrintInfo();

  for (const State* state : *automaton_->states()) {
      if (GetState(state, tick) != kInvalidPos) {
        if (tick == 0) {
          cout << "  node [" ACTIVE_STYLE_INITIAL "]; ";
        } else {
          cout << "  node [" ACTIVE_STYLE_FUTURE "]; ";
        }
      } else {
        cout << "  node [" INACTIVE_STYLE "]; ";
      }
        cout << state->index() << ";\n";
  }
  cout << "  // Transitions.\n";
  cout << "  node [" INACTIVE_STYLE "];\n";
  for (const State* state : *automaton_->states()) {
    bool active_state = GetState(state, tick) != kInvalidPos;
    for (const Regexp* regexp : *state->from()) {
      cout << "  " << state->index() << " -> " << regexp->exit()->index()
          << " [label=\"";
      printer.Visit(regexp);
      cout << "\"";
      if (active_state && (tick == 0) && (-1 != regexp->Match(current_pos_))) {
        cout << "," ACTIVE_STYLE_TRANSITION;
      }
      cout << "];\n";
    }
  }
  cout << "}\n";
}


} }  // namespace regit::internal
