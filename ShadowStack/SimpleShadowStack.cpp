#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_point.h"

using namespace Dyninst;

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

int AllocateShadowStack(BPatch_binaryEdit *app, BPatch_image* image) {
    auto init_func = FindFunctionByName(image, "_start");
    if (init_func == nullptr) {
        fprintf(stderr, "Cannot find _start function\n");
        return -1;
    }
    
    // Construct a piece of code that calls the function
    // that allocates the shadow stack.
    BPatch_Vector<BPatch_snippet*> args;
    auto stack_init_fn = FindFunctionByName(image, "shadow_stack_init");
    if (stack_init_fn == nullptr) {
        fprintf(stderr, "Cannot find shadow_stack_init\n");
        return -1;
    }
    BPatch_funcCallExpr stack_init(*stack_init_fn, args);
  
    // Get function entry instrumentation points
    std::vector<BPatch_point*>* entries = init_func->findPoint(BPatch_entry);

    // Insert the function call snippet to all function entry points
    BPatchSnippetHandle* handle = nullptr;
    handle = app->insertSnippet(stack_init, *entries, BPatch_callBefore, BPatch_firstSnippet);
    if (handle == nullptr) {
        fprintf(stderr, "Cannot insert the snippet\n");
        return -1;
    }
    return 0;
}

int Instrument(BPatch_binaryEdit *app, BPatch_image* image) {
    std::vector<BPatch_function*>* funcs = image->getProcedures();

    // Load instrumentation library
    if (!app->loadLibrary("libshadowstack.so")) {
        fprintf(stderr, "Cannot open instrumentation library\n");
        return -1;
    };

    for (auto f : *funcs) {
        // Function entry instrumentation 
        BPatch_Vector<BPatch_snippet*> args;

        // We will need the return address 
        args.push_back(new BPatch_retAddrExpr());
        auto stack_push_fn = FindFunctionByName(image, "shadow_stack_push");
        if (stack_push_fn == nullptr) {
            fprintf(stderr, "Cannot find shadow_stack_push\n");
            return -1;
        }

        BPatch_funcCallExpr stack_push(*stack_push_fn, args);
        
        // Get function entry instrumentation points
        std::vector<BPatch_point*>* entries = f->findPoint(BPatch_entry);
        
        // Insert the function call snippet to all function entry points
        BPatchSnippetHandle* handle = nullptr;
        handle = app->insertSnippet(stack_push, *entries, BPatch_callBefore, BPatch_lastSnippet);

        // Function exit instrumentation
        auto stack_pop_fn = FindFunctionByName(image, "shadow_stack_pop");
        if (stack_pop_fn == nullptr) {
            fprintf(stderr, "Cannot find shadow_stack_pop\n");
            return -1;
        }
        BPatch_funcCallExpr stack_pop(*stack_pop_fn, args);
        
        // Get function entry instrumentation points
        std::vector<BPatch_point*>* exits = f->findPoint(BPatch_exit);
        
        // Insert the function call snippet to all function exit points
        handle = app->insertSnippet(stack_pop, *exits, BPatch_callAfter, BPatch_lastSnippet);
    }
    return 0;
}


int main(int argc, char **argv) {
    // Open the input binary
    BPatch_binaryEdit* app = bpatch.openBinary(argv[1]);

    BPatch_image* image = app->getImage();
    if (Instrument(app, image)) {
        fprintf(stderr, "Cannot instrument the binary\n");
        return 1;
    }
    if (AllocateShadowStack(app, image) < 0) {
        fprintf(stderr, "Cannot allocate shadow stack\n");
        return 1;
    }

    // Write the instrumented binary to disk
    app->writeFile(argv[2]);
}
