#include "Symtab.h"
#include "Symbol.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::SymtabAPI;

int main(int argc, char ** argv) {
    Symtab *sym;
    if (!Symtab::openFile(sym, string(argv[1]))) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        exit(-1);
    }

    vector<ExceptionBlock*> exs;
    sym->getAllExceptions(exs);
    for (auto b : exs) {
        printf("tryStart = %lx, tryEnd = %lx, catchStart = %lx\n", b->tryStart(), b->tryEnd(), b->catchStart());
    }
}
