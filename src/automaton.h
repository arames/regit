#ifndef REGIT_AUTOMATON_H_
#define REGIT_AUTOMATON_H_

#include <map>
#include <vector>

#include "regexp.h"
#include "regexp_visitor.h"

namespace regit {
namespace internal {

typedef int state_t;

class RegexpIndexer;

class Automaton {
 public:
  explicit Automaton(Regexp* regexp)
      : entry_state_(0), exit_state_(0), last_state_(0) {
    BuildFrom(regexp);
  }

  void BuildFrom(Regexp* regexp);

  void Print() const;

 private:
  struct Transition {
    const Regexp* regexp;
    state_t from;
    state_t to;
  };
  multimap<state_t, struct Transition> transitions_from_;
  multimap<state_t, struct Transition> transitions_to_;
  map<const Regexp*, struct Transition> transitions_;

  state_t entry_state_;
  state_t exit_state_;
  state_t last_state_;

  friend class RegexpIndexer;
};


class RegexpIndexer {
 public:
  explicit RegexpIndexer(Automaton* automaton) : automaton_(automaton) {}

  void Visit(const Regexp* regexp,
             int entry_state = kNoSpecifiedState,
             int exit_state = kNoSpecifiedState) {
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

  void VisitRegexp(const Regexp* regexp,
                   int entry_state = kNoSpecifiedState,
                   int exit_state = kNoSpecifiedState);

#define DECLARE_REGEXP_LEAF_VISITORS(RegexpType)                               \
  void Visit##RegexpType(const RegexpType* regexp,                             \
                         int entry_state = kNoSpecifiedState,                  \
                         int exit_state = kNoSpecifiedState) {                 \
    VisitRegexp(regexp, entry_state, exit_state);                              \
  }
  LIST_LEAF_REGEXP_TYPES(DECLARE_REGEXP_LEAF_VISITORS)
#undef DECLARE_REGEXP_LEAF_VISITORS

#define DECLARE_REGEXP_FLOW_VISITORS(RegexpType)                               \
  void Visit##RegexpType(const RegexpType* regexp,                             \
                         int entry_state = kNoSpecifiedState,                  \
                         int exit_state = kNoSpecifiedState);
  LIST_FLOW_REGEXP_TYPES(DECLARE_REGEXP_FLOW_VISITORS)
#undef DECLARE_REGEXP_FLOW_VISITORS

  static constexpr int kNoSpecifiedState = -1;

 private:
  Automaton* automaton_;
};

} }  // namespace regit::internal

#endif  // REGIT_AUTOMATON_H_
