#ifndef REGIT_AUTOMATON_H_
#define REGIT_AUTOMATON_H_

#include <vector>

#include "regexp.h"
#include "regexp_visitor.h"
#include "regit.h"

namespace regit {
namespace internal {

class RegexpIndexer;


class State {
 public:
  State() {}

  int index() const { return index_; }
  void set_index(int index) { index_ = index; }

  const vector<const Regexp*>* from() const {
    return &from_;
  }
  const vector<const Regexp*>* to() const {
    return &to_;
  }

 private:
  int index_;
  vector<const Regexp*> from_;
  vector<const Regexp*> to_;

  friend class RegexpIndexer;
};


class Automaton {
 public:
  explicit Automaton(Regexp* regexp)
      : entry_state_(nullptr), exit_state_(nullptr), last_state_(nullptr),
        max_transition_match_length_(0),
        status_(kSuccess) {
    BuildFrom(regexp);
  }
  ~Automaton() {
    for (State* state : states_) {
      delete state;
    }
  }

  void BuildFrom(Regexp* regexp);

  void IndexStates();

  State* NewState();

  int NStates() const { return states_.size(); }
  const State* entry_state() const { return entry_state_; }
  const State* exit_state() const { return exit_state_; }
  const vector<State*>* states() const { return &states_; }

  int max_transition_match_length() const {
    return max_transition_match_length_;
  }
  void UpdateMaxTransitionMatchLength(int length) {
    max_transition_match_length_ = max(max_transition_match_length_, length);
  }

  Status status() const { return status_; }

  void Print() const;
  void PrintInfo() const;

 private:
  State* entry_state_;
  State* exit_state_;
  State* last_state_;

  vector<State*> states_;

  int max_transition_match_length_;

  Status status_;

  friend class RegexpIndexer;
};


class RegexpIndexer {
 public:
  explicit RegexpIndexer(Automaton* automaton) : automaton_(automaton) {}

  void Visit(Regexp* regexp,
             State* entry_state = nullptr,
             State* exit_state = nullptr) {
    switch (regexp->type()) {
#define TYPE_CASE(RegexpType)                                                  \
      case k##RegexpType:                                                      \
        Visit##RegexpType(regexp->As##RegexpType(),                            \
                          entry_state,                                         \
                          exit_state);                                         \
        break;
      LIST_REAL_REGEXP_TYPES(TYPE_CASE)
#undef TYPE_CASE
      default:
        UNREACHABLE();
        return;
    }
  }

  void VisitRegexp(Regexp* regexp,
                   State* entry_state = nullptr,
                   State* exit_state = nullptr);

#define DECLARE_REGEXP_LEAF_VISITORS(RegexpType)                               \
  void Visit##RegexpType(RegexpType* regexp,                                   \
                         State* entry_state = nullptr,                         \
                         State* exit_state = nullptr) {                        \
    VisitRegexp(regexp, entry_state, exit_state);                              \
  }
  LIST_LEAF_REGEXP_TYPES(DECLARE_REGEXP_LEAF_VISITORS)
#undef DECLARE_REGEXP_LEAF_VISITORS

#define DECLARE_REGEXP_FLOW_VISITORS(RegexpType)                               \
  void Visit##RegexpType(RegexpType* regexp,                                   \
                         State* entry_state = nullptr,                         \
                         State* exit_state = nullptr);
  LIST_FLOW_REGEXP_TYPES(DECLARE_REGEXP_FLOW_VISITORS)
#undef DECLARE_REGEXP_FLOW_VISITORS

 private:
  Automaton* automaton_;
};


class Simulation {
 public:
  explicit Simulation(const Automaton* automaton)
      : automaton_(automaton),
        n_states_(automaton->NStates()),
        n_ticks_(automaton->max_transition_match_length() + 1),
        current_pos_(kInvalidPos),
        data_(nullptr) {
    data_ = reinterpret_cast<pos_t*>(malloc(ComputeDataSize()));
    memset(data_, 0, ComputeDataSize());
  }
  ~Simulation() {
    free(data_);
  }

  bool MatchFull(const char* text, size_t text_size);
  bool MatchAnywhere(Match* match, const char* text, size_t text_size);
  bool MatchFirst(Match* match, const char* text, size_t text_size);

  size_t ComputeTickSize() const {
    return automaton_->NStates() * sizeof(pos_t);
  }
  size_t ComputeDataSize() const {
    return n_ticks_ * ComputeTickSize();
  }

  pos_t* StatePointer(const State* state, int tick) const {
    return StatePointer(state->index(), tick);
  }
  void SetState(pos_t pos, const State* state, int tick) const {
    *StatePointer(state, tick) = pos;
  }
  pos_t GetState(const State* state, int tick) const {
    return *StatePointer(state, tick);
  }

  void InvalidateState(const State* state, int tick) const {
    STATIC_ASSERT(kInvalidPos == nullptr);
    SetState(0, state, tick);
  }
  void InvalidateTick(int tick) const {
    STATIC_ASSERT(kInvalidPos == nullptr);
    memset(StatePointer(0, tick), 0, ComputeTickSize());
  }

  void Advance(int ticks) {
    current_tick_ = (current_tick_ + ticks) % n_ticks_;
    ASSERT(remaining_text_size() > 0);
    ASSERT(current_pos_ < text_end_);
    current_pos_++;
  }

  void InvalidateStatesBefore(pos_t pos);

  int Index(pos_t pos) const { return pos - text_; }
  int CurrentIndex() const { return Index(current_pos_); }
  void Print(int tick) const;
  void Print() const {
    for (int i = 0;
         i < n_ticks_ && static_cast<size_t>(i) <= remaining_text_size();
         i++) {
      Print(i);
    }
    cout << "// End of index\n";
  }

  size_t remaining_text_size() const { return text_end_ - current_pos_; }

  Status status() const { return status_; }

 private:
  pos_t* StatePointer(int state_index, int tick) const {
    int t = (current_tick_ + tick) % n_ticks_;
    return data_ + t * automaton_->NStates() + state_index;
  }

  const Automaton* automaton_;
  const int n_states_;
  const int n_ticks_;

  int current_tick_;
  pos_t text_;
  pos_t text_end_;
  pos_t current_pos_;

  pos_t* data_;
  Status status_;
};


} }  // namespace regit::internal

#endif  // REGIT_AUTOMATON_H_
