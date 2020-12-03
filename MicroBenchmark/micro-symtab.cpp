// A simple little microbenchmark

#include <iostream>
#include "Symtab.h"

#include "time.h"

using namespace Dyninst;
using namespace SymtabAPI;

float timeDiff(struct timespec &t1, struct timespec &t0) {
    return ((t1.tv_sec - t0.tv_sec) * 1000000000.0 + (t1.tv_nsec - t0.tv_nsec)) / 1000000000.0;
}



int main(int argc, const char** argv) {
  if(argc < 2) {
    std::cerr << "Not enough arguments!\n";
    return 2;
  }
  struct timespec t0;
  clock_gettime(CLOCK_MONOTONIC, &t0);

  Symtab* symtab = NULL;
  if(!Symtab::openFile(symtab, argv[1])) {
    std::cerr << "Error opening file!\n";
    return 1;
  }
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);

  Module* defmod = NULL;
  if(!symtab->findModuleByName(defmod, argv[1])) {
    std::cerr << "Unable to find default module!\n";
    return 1;
  }

  symtab->parseTypesNow();
  struct timespec t2;
  clock_gettime(CLOCK_MONOTONIC, &t2);
  printf("Open file time: %.2lf\n", timeDiff(t1, t0));
  printf("DWARF parsing time: %.2lf\n", timeDiff(t2, t1));
  printf("Total time: %.2lf\n", timeDiff(t2, t0));
  return 0;
}
