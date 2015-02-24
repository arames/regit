#ifndef REGIT_AUTOMATON_H_
#define REGIT_AUTOMATON_H_

#include <map>
#include <vector>

#include "regexp.h"
#include "regexp_visitor.h"
#include "regit.h"

namespace regit {
namespace internal {

typedef int state_t;

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

  Status status() const { return status_; }

  void Print() const;

 private:
  State* entry_state_;
  State* exit_state_;
  State* last_state_;

  vector<State*> states_;

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

} }  // namespace regit::internal

#endif  // REGIT_AUTOMATON_H_
