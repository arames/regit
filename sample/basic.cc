#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>

#include "regit.h"

using namespace std;

void usage(const char* prog) {
  printf("Usage: %s [--help] REGEXP TEXT\n", prog);
}

int main(int argc, const char* argv[]) {
  for (int i = 0; i < argc; i++) {
    if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0)) {
      usage(argv[0]);
      exit(0);
    }
  }
  if (argc != 3) {
    usage(argv[0]);
    exit(1);
  }

  const char* regexp = argv[1];
  const char* text = argv[2];

  regit::Regit re(regexp);
  if (re.status() != regit::kSuccess) {
    return EXIT_FAILURE;
  }

  bool match = re.MatchFull(text);
  if (match) {
    printf("match\n");
  } else {
    printf("NO match\n");
  }

  return EXIT_SUCCESS;
}
