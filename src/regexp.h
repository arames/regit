#ifndef REGIT_REGEXP_H_
#define REGIT_REGEXP_H_

#include <iostream>
#include <vector>

#include "globals.h"

using namespace std;

namespace regit {
namespace internal {

class State;

// The enumeration order of the regexp types matters.  See aliases in the enum
// below.

#define LIST_MATCHING_REGEXP_TYPES(M)                                          \
  M(Period)                                                                    \
  M(MultipleChar)

#define LIST_CONTROL_REGEXP_TYPES(M)                                           \
  M(Epsilon)

#define LIST_LEAF_REGEXP_TYPES(M)                                              \
  LIST_MATCHING_REGEXP_TYPES(M)                                                \
  LIST_CONTROL_REGEXP_TYPES(M)

#define LIST_FLOW_REGEXP_TYPES(M)                                              \
  M(Concatenation)                                                             \
  M(Alternation)

// Real regular expression can appear in a regular expression tree, after
// parsing has finished and succeded.
// There is a sub class of Regexp for each of these.
#define LIST_REAL_REGEXP_TYPES(M)                                              \
  LIST_LEAF_REGEXP_TYPES(M)                                                    \
  LIST_FLOW_REGEXP_TYPES(M)

// These regular expressions are only used while parsing.
#define LIST_PARSETIME_REGEXP_TYPES(M)                                         \
  M(LeftParenthesis)                                                           \
  M(AlternateBar)

#define LIST_REGEXP_TYPES(M)                                                   \
  LIST_REAL_REGEXP_TYPES(M)                                                    \
  LIST_PARSETIME_REGEXP_TYPES(M)

#define ENUM_REGEXP_TYPES(RegexpType) k##RegexpType,
enum RegexpType {
  LIST_REGEXP_TYPES(ENUM_REGEXP_TYPES)
  // Aliases.
  kFirstMatchingRegexp = kMultipleChar,
  kLastMatchingRegexp = kMultipleChar,
  kFirstControlRegexp = kEpsilon,
  kLastControlRegexp = kEpsilon,
  kFirstLeafRegexp = kFirstMatchingRegexp,
  kLastLeafRegexp = kLastControlRegexp,
  kFirstFlowRegexp = kConcatenation,
  kLastFlowRegexp = kAlternation,
  kFirstMarker = kLeftParenthesis
};
#undef ENUM_REGEXP_TYPES


// Intermediate classes used for convenience.
#define LIST_INTERMEDIATE_REGEXP_TYPES(M)                                      \
  M(LeafRegexp)                                                                \
  M(ControlRegexp)                                                             \
  M(FlowRegexp)


// Forward declaration of real regexp classes.
class Regexp;
#define FORWARD_DECLARE(RegexpType) class RegexpType;
LIST_REAL_REGEXP_TYPES(FORWARD_DECLARE)
LIST_INTERMEDIATE_REGEXP_TYPES(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class RegexpVisitor;


// Regexps ---------------------------------------------------------------------

// The base class for Regexps.
// The parser builds a tree of Regexps, that is then passed to a code generator
// to generate some code matching the respresented regular expression.
class Regexp {
 public:
  explicit Regexp(RegexpType type)
      : type_(type), entry_(nullptr), exit_(nullptr) {}
  virtual ~Regexp() {}

#define DECLARE_IS_REGEXP_HELPERS(RegexpType)                                  \
  bool Is##RegexpType() const { return type_ == k##RegexpType; }
  LIST_REGEXP_TYPES(DECLARE_IS_REGEXP_HELPERS)
#undef DECLARE_IS_REGEXP_HELPERS

#define DECLARE_IS_REGEXP_HELPERS(RegexpType)                                  \
  bool Is##RegexpType() const {                                                \
    return kFirst##RegexpType <= type_ && type_ <= kLast##RegexpType;          \
  }
  LIST_INTERMEDIATE_REGEXP_TYPES(DECLARE_IS_REGEXP_HELPERS)
#undef DECLARE_IS_REGEXP_HELPERS

#define DECLARE_CAST(RegexpType)                                               \
  RegexpType* As##RegexpType() {                                               \
    ASSERT(Is##RegexpType());                                                  \
    return reinterpret_cast<RegexpType*>(this);                                \
  }                                                                            \
  const RegexpType* As##RegexpType() const {                                   \
    ASSERT(Is##RegexpType());                                                  \
    return reinterpret_cast<const RegexpType*>(this);                          \
  }
  LIST_REAL_REGEXP_TYPES(DECLARE_CAST)
  LIST_INTERMEDIATE_REGEXP_TYPES(DECLARE_CAST)
#undef DECLARE_CAST

  virtual void Accept(RegexpVisitor* visitor) const  {
    UNUSED(visitor);
    UNREACHABLE();
  }
#define DECLARE_ACCEPT(Name)                                                   \
  void Accept(RegexpVisitor* visitor) const OVERRIDE

  virtual int Match(const char* string) const {
    UNUSED(string);
    UNREACHABLE();
    return -1;
  }

  virtual int MatchLength() const {
    UNREACHABLE();
    return -1;
  }

  // Left parenthesis and vertical bar are markers for the parser.
  bool IsMarker() const { return type_ >= kFirstMarker; }

  // Debug helpers.
  void Print() const;

  // Accessors.
  RegexpType type() const { return type_; }

  State* entry() const { return entry_; }
  void set_entry(State* entry) { entry_ = entry; }
  State* exit() const { return exit_; }
  void set_exit(State* exit) { exit_ = exit; }

 protected:
  const RegexpType type_;
  State* entry_;
  State* exit_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Regexp);
};


class LeafRegexp : public Regexp {
 public:
  explicit LeafRegexp(RegexpType type) : Regexp(type) {}
};


class MultipleChar : public LeafRegexp {
 public:
  MultipleChar() : LeafRegexp(kMultipleChar) {
    chars_.push_back('\0');
  }

  int Match(const char* string) const OVERRIDE {
    if (!strncmp(Chars(), string, NChars())) {
      return NChars();
    } else {
      return -1;
    }
  }

  bool IsFull() {
#ifdef MC_MAX_ONE_CHAR
    static constexpr size_t kMaxMCLength= 1;
# else
    static constexpr size_t kMaxMCLength= 32;
#endif
    ASSERT(NChars() <= kMaxMCLength);
    return NChars() == kMaxMCLength;
  }

  void PushChar(char c) {
    ASSERT(!IsFull());
    chars_.back() = c;
    chars_.push_back('\0');
  }

  const char* Chars() const { return &chars_[0]; }
  size_t NChars() const { return chars_.size() - 1; }

  int MatchLength() const OVERRIDE { return NChars(); }

  DECLARE_ACCEPT(MultipleChar);

 protected:
  vector<char> chars_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MultipleChar);
};


class Period : public LeafRegexp {
 public:
  Period() : LeafRegexp(kPeriod), posix_(false) {}
  explicit Period(bool posix) : LeafRegexp(kPeriod), posix_(posix) {}

  int Match(const char* string) const OVERRIDE {
    if (*string != '\n' && *string != '\r') {
      return 1;
    } else {
      return -1;
    }
  }

  bool posix() const { return posix_; }

  int MatchLength() const OVERRIDE { return 1; }

  DECLARE_ACCEPT(Period);

 private:
  // When true, the only character not matched by a '.' is the end-of-string
  // delimiter.
  bool posix_;
  DISALLOW_COPY_AND_ASSIGN(Period);
};


// Control regexp don't match characters from the input. They check for
// conditions or/and have side effects.
class ControlRegexp : public Regexp {
 protected:
  explicit ControlRegexp(RegexpType type) : Regexp(type) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ControlRegexp);
};


class Epsilon : public ControlRegexp {
 public:
  Epsilon() : ControlRegexp(kEpsilon) {}

  DECLARE_ACCEPT(Epsilon);

 private:
  DISALLOW_COPY_AND_ASSIGN(Epsilon);
};


class FlowRegexp : public Regexp {
 public:
  explicit FlowRegexp(RegexpType regexp_type) : Regexp(regexp_type) {}
  virtual ~FlowRegexp() {
    vector<Regexp*>::iterator it;
    for (it = sub_regexps_.begin(); it < sub_regexps_.end(); it++) {
      (*it)->~Regexp();
    }
  }

  void Append(Regexp* regexp) {
    sub_regexps_.push_back(regexp);
  }

  vector<Regexp*>* sub_regexps() { return &sub_regexps_; }
  vector<Regexp*> const * sub_regexps() const { return &sub_regexps_; }

 protected:
  vector<Regexp*> sub_regexps_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FlowRegexp);
};


class Concatenation : public FlowRegexp {
 public:
  Concatenation() : FlowRegexp(kConcatenation) {}

  void Concatenate(Regexp* regexp) { Append(regexp); }

  DECLARE_ACCEPT(Concatenation);

 private:
  DISALLOW_COPY_AND_ASSIGN(Concatenation);
};


class Alternation : public FlowRegexp {
 public:
  Alternation() : FlowRegexp(kAlternation) {}

  void Alternate(Regexp* regexp) { Append(regexp); }

  DECLARE_ACCEPT(Alternation);

 private:
  DISALLOW_COPY_AND_ASSIGN(Alternation);
};


} }  // namespace regit::internal

#endif  // REGIT_REGEXP_H_

