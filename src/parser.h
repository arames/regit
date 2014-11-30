#ifndef REGIT_PARSER_H_
#define REGIT_PARSER_H_

#include <stack>

#include "regit.h"
#include "regexp.h"

namespace regit {
namespace internal {

class Parser {
 public:
  Parser(const Options* options) : options_(options), status_(Success) {}

  // The regexp must be '\0' terminated.
  Regexp* Parse(const char* regexp, size_t regexp_size);

  void ConsumeAlternateBar();
  void ConsumeChar();
  void ConsumeLeftParenthesis();
  void ConsumeRightParenthesis();

  void DoAlternation();
  void DoConcatenation();
  void DoFinish();

  void Advance(size_t n);
  size_t current_index() { return current_ - regexp_string_; }

  // Stack access helpers --------------------------------------------
 
  // tos = top of stack.
  Regexp* tos() { return !stack_.empty() ? stack_.back() : nullptr; }

  void PushRegexp(Regexp* regexp);
  Regexp* PopRegexp();

  void PopLeftParenthesis();

  bool markers_on_stack();
  vector<Regexp*>::iterator top_marker();


  // Error signaling -------------------------------------------------

  // TODO: Error reporting should not just print to stdout. Should we use an
  // errno-like system?
  void ParseError(const char* format, ...);

  void Unexpected() {
    return ParseError("unexpected character %c\n", *current_);
  }

  void Expected(const char* expected) {
    return ParseError("expected: %s\n", expected);
  }


  // Debugging -------------------------------------------------------
  void PrintStatus();

 private:
  const char* regexp_string_;
  const char* current_;
  size_t remaining_size_;

  vector<Regexp*> stack_;
  // Offsets of specific markers in the stack.
  std::stack<size_t> open_parenthesis_;
  std::stack<size_t> alternate_bars_;

  const Options* options_;
  Status status_;
};

inline void Parser::ConsumeAlternateBar() {
  DoConcatenation();
  alternate_bars_.push(stack_.size());
  PushRegexp(new Regexp(kAlternateBar));
  Advance(1);
}

inline void Parser::ConsumeLeftParenthesis() {
  open_parenthesis_.push(stack_.size());
  PushRegexp(new Regexp(kLeftParenthesis));
  Advance(1);
}

inline void Parser::Advance(size_t n = 1) {
  ASSERT(remaining_size_ >= n);
  current_ += n;
  remaining_size_ -= n;
}

inline void Parser::PushRegexp(Regexp* regexp) {
  stack_.push_back(regexp);
}

inline Regexp* Parser::PopRegexp() {
  Regexp* r = stack_.back();
  stack_.pop_back();
  return r;
}

inline void Parser::PopLeftParenthesis() {
  ASSERT(tos()->IsLeftParenthesis());
  PopRegexp();
  ASSERT(stack_.size() == open_parenthesis_.top());
  open_parenthesis_.pop();
}

inline bool Parser::markers_on_stack() {
  return !open_parenthesis_.empty() || !alternate_bars_.empty();
}

inline vector<Regexp*>::iterator Parser::top_marker() {
  int max_paren = open_parenthesis_.empty() ? 0 : open_parenthesis_.top();
  int max_bar = alternate_bars_.empty() ? 0 : alternate_bars_.top();
  return stack_.begin() + max(max_paren, max_bar);
}

} }  // namespace regit::internal

#endif  // REGIT_PARSER_H_

