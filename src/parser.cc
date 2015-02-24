#include <iostream>
#include <stdlib.h>

#include "globals.h"
#include "parser.h"

namespace regit {
namespace internal {

Regexp* Parser::Parse(const char* regexp, size_t regexp_size) {
  ASSERT(regexp != nullptr);
  regexp_string_ = regexp;
  current_ = regexp;
  remaining_size_ = regexp_size;

  while (*current_) {
    switch (*current_) {
      case '(':
        ConsumeLeftParenthesis();
        break;

      case ')':
        ConsumeRightParenthesis();
        break;

      case '|':
        ConsumeAlternateBar();
        break;

      case '\\': {
        Advance(1);
        switch (*current_) {
          case '$':
          case '(':
          case ')':
          case '*':
          case '+':
          case '.':
          case '[':
          case ']':
          case '^':
          case '{':
          case '|':
          case '}':
          case '\\':
            ConsumeChar();
            break;

          default:
            Unexpected();
            return nullptr;
        }
        break;
      }

      case '{':
        ParseError("Unsupported repetition.");
        return nullptr;

      case '.':
        PushRegexp(new Period(options_->posix_period_));
        Advance(1);
        break;

      case '*':
        ParseError("Unsupported Kleene operator.");
        return nullptr;

      case '+':
        ParseError("Unsupported '+' operator.");
        return nullptr;

      case '?':
        ParseError("Unsupported '?' operator.");
        return nullptr;

      case '^':
        ParseError("Unsupported '^' anchor.");
        return nullptr;

      case '$':
        ParseError("Unsupported '$' anchor.");
        return nullptr;


      case '[':
        ParseError("Character classes are not supported.");
        return nullptr;
      case ']':
        UNREACHABLE();
        break;

      default:
        // Standard character.
        ConsumeChar();
    }

    if (status_ != Success) {
      return nullptr;
    }
  }

  DoFinish();

  if (status_ != Success) {
    return nullptr;
  } else {
    Regexp* result = stack_.at(0);
    if (FLAG_print_re_tree) {
      result->Print();
    }
    return result;
  }
}


void Parser::ConsumeChar() {
  MultipleChar* mc;

  if (tos() && tos()->IsMultipleChar()) {
    mc = reinterpret_cast<MultipleChar*>(tos());
    if (!mc->IsFull()) {
      mc->PushChar(*current_);
      Advance(1);
      return;
    }
  }

  mc = new MultipleChar();
  mc->PushChar(*current_);
  PushRegexp(mc);
  Advance(1);
}


void Parser::ConsumeRightParenthesis() {
  if (open_parenthesis_.empty()) {
    ParseError("Unmatched closing parenthesis.");
    return;
  }

  DoAlternation();
  if (tos()->IsLeftParenthesis()) {
    PopLeftParenthesis();
  } else {
    Regexp* concat = PopRegexp();
    PopLeftParenthesis();
    PushRegexp(concat);
  }
  Advance(1);
}


void Parser::DoAlternation() {
  // The stack looks like:
  //   ... ( regexp1 | regexp2 | regexp3 regexp4 regexp5

  DoConcatenation();

  // Now the stack looks like:
  //   ... ( regexp1 | regexp2 | regexp_3_4_5
  // Collapse the alternations of regular expressions into one Alternation.

  vector<Regexp*> alternated_regexps;

  vector<Regexp*>::reverse_iterator it = stack_.rbegin();
  for (it = stack_.rbegin();
       it != stack_.rend() && !(*it)->IsLeftParenthesis();
       it++) {
    if ((*it)->IsAlternateBar()) {
      // Reverse iterators point after the element.
      ASSERT(!alternate_bars_.empty() &&
             (static_cast<size_t>(distance(stack_.begin(), it.base() - 1)) ==
              alternate_bars_.top()));
      alternate_bars_.pop();
      continue;
    }
    alternated_regexps.push_back(*it);
  }

  stack_.erase(it.base(), stack_.end());

  if (FLAG_parser_opt && (alternated_regexps.size() <= 1)) {
    // Avoid trivial alternations of zero or one element.
    if (alternated_regexps.empty()) {
      return;
    } else {
      PushRegexp(alternated_regexps.front());
    }
  } else {
    Alternation* alternation = new Alternation();
    alternation->sub_regexps()->assign(alternated_regexps.begin(),
                                       alternated_regexps.end());
    PushRegexp(alternation);
  }
}


void Parser::DoConcatenation() {
  vector<Regexp*>::iterator first_re;

  if (markers_on_stack()) {
    first_re = top_marker() + 1;
  } else {
    first_re = stack_.begin();
  }

  if (first_re >= stack_.end()) {
    return;
  }

  if (FLAG_parser_opt &&
      (distance(first_re, stack_.end()) <= 1)) {
    // Avoid trivial concatenations of zero or one elements.
    return;
  }

  ASSERT(first_re < stack_.end());
  Concatenation* concatenation = new Concatenation();
  concatenation->sub_regexps()->insert(
      concatenation->sub_regexps()->end(), first_re, stack_.end());

  stack_.erase(first_re, stack_.end());
  PushRegexp(concatenation);
}


void Parser::DoFinish() {
  // Fold regexps on the stack.
  DoAlternation();

  // The stack should now only contain the root regexp.
  if (stack_.size() > 1) {
    ParseError("Missing %d right-parenthis ')'.\n", open_parenthesis_.size());
  }
}


void Parser::ParseError(const char *format, ...) {
  unsigned index = current_ - regexp_string_;
  fprintf(stderr, "Error parsing at index %d\n%s\n%s^ \n",
         index, regexp_string_, string(index, ' ').c_str());
  va_list argptr;
  va_start(argptr, format);
  vfprintf(stderr, format, argptr);
  fprintf(stderr, "\n");
  va_end(argptr);
  status_ = ParserError;
}


void Parser::PrintStatus() {
  cout << "Parser {" << endl;
  vector<Regexp*>::iterator it;
  for (Regexp* re : stack_) {
    re->Print();
  }
  cout << "}" << endl;
  if (!open_parenthesis_.empty()) {
    cout << "top open parenthesis at index " << open_parenthesis_.top() << endl;
  }
  if (!alternate_bars_.empty()) {
    cout << "top alternate bar at index " << alternate_bars_.top() << endl;
  }
  cout << "--------------------------{" << endl;
}


} }  // namespace regit::internal
