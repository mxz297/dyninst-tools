#include "CodeSource.h"
#include "CodeObject.h"
#include "CFG.h"
#include "Symtab.h"
#include "time.h"
using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;

float timeDiff(struct timespec &t1, struct timespec &t0) {
    return ((t1.tv_sec - t0.tv_sec) * 1000000000.0 + (t1.tv_nsec - t0.tv_nsec)) / 1000000000.0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary>\n", argv[0]);
        exit(-1);
    }

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    SymtabAPI::Symtab *symObj = NULL;
    if (!SymtabAPI::Symtab::openFile(symObj, argv[1])) {
        fprintf(stderr, "Cannot open file %s\n", argv[1]);
        return -1;
    }
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);

    SymtabCodeSource *sts = new SymtabCodeSource(symObj);
    CodeObject *co = new CodeObject(sts);
    co->parse();
    struct timespec t2;
    clock_gettime(CLOCK_MONOTONIC, &t2);

    printf("SymtabAPI time: %.2lf\n", timeDiff(t1, t0));
    printf("ParseAPI time: %.2lf\n", timeDiff(t2, t1));
    printf("Total time: %.2lf\n", timeDiff(t2, t0));

/*
    delete co;
    delete sts;
    delete symObj;
    */
}
