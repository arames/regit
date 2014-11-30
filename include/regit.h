#ifndef REGIT_H_
#define REGIT_H_

#include <string>

using namespace std;

namespace regit {

// Error status returned by regit.
// Upon error, the regit_status_string is updated with an error message.
enum Status {
  Success = 0,
  ParserError,
};

extern char* const regit_status_string;

namespace internal {
// Structure used to track internal compilation information.
// A forward declaration is required here to reference it from class Regit.
class RegexpInfo;
}

class Regit {
 public:
  // The regexp must be '\0'-terminated.
  explicit Regit(const char* regexp);
  explicit Regit(const string& regexp);
  ~Regit();

  void Compile();

 private:
  char const * const regexp_;
  size_t regexp_size_;

 public:
  internal::RegexpInfo* rinfo_;
};

}  // namespace regit

#endif  // REGIT_H_
