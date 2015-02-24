#ifndef REGIT_H_
#define REGIT_H_

#include <string>

using namespace std;

namespace regit {

// Error status used by regit.
enum Status {
  kSuccess,
  kOutOfMemory,
  kParserError,
  kParserUnsupported,
  kParserUnexpected,
  kParserMissingLeftParenthesis,
  kParserMissingRightParenthesis
};

namespace internal {
// Structure used to track internal compilation information.
// A forward declaration is required here to reference it from class Regit.
class RegexpInfo;
}


class Options {
 public:
  // Options:
  //   posix_period:
  //     When unset, period ('.') does not match newline characters. When set,
  //     they match everything except end-of-string delimiter
  Options(bool posix_period = false) :
      posix_period_(posix_period) {}
  bool posix_period_;
};


extern const Options regit_default_options;


class Regit {
 public:
  // The regexp must be '\0'-terminated.
  explicit Regit(const char* regexp);
  explicit Regit(const string& regexp);
  ~Regit();

  void Compile(const Options* options = &regit_default_options);

  Status status() const { return status_; }

 private:
  char const * const regexp_;
  size_t regexp_size_;

  // If anything went wrong, this should indicate what did.
  Status status_;

 public:
  internal::RegexpInfo* rinfo_;
};

}  // namespace regit

#endif  // REGIT_H_
