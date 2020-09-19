#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_image.h"
#include "BPatch_module.h"
using namespace Dyninst;
using namespace SymtabAPI;

// BPatch is the top level object in Dyninst
BPatch bpatch;

BPatch_function* FindFunctionByName(BPatch_image* image, std::string name) {
  BPatch_Vector<BPatch_function*> funcs;
  if (image->findFunction(name.c_str(), funcs,
                          /* showError */ true,
                          /* regex_case_sensitive */ true,
                          /* incUninstrumentable */ true) == nullptr ||
      !funcs.size() || funcs[0] == nullptr) {
    return nullptr;
  }
  return funcs[0];
}

int main(int argc, char **argv) {    
    BPatch_process* app = bpatch.processCreate(argv[1], (const char**)(&argv[1]));
    BPatch_image* appImage = app->getImage();
    app->loadLibrary("libwrap.so");
    
    BPatch_function *malloc_wrappee = FindFunctionByName(appImage, "malloc");
    BPatch_function *malloc_wrapper = FindFunctionByName(appImage, "malloc_wrapper");
    Symtab *symtab = SymtabAPI::convert(malloc_wrapper->getModule()->getObject());
    std::vector<Symbol *> syms;
    symtab->findSymbol(syms, "origMalloc",
            Symbol::ST_UNKNOWN, // Don’t specify type
            mangledName, // Look for raw symbol name
            false, // Not regular expression
            false, // Don’t check case
            true); // Include undefined symbols
    app->wrapFunction(malloc_wrappee, malloc_wrapper, syms[0]);
    app->continueExecution();
    while (!app->isTerminated())
        bpatch.waitForStatusChange();
}


