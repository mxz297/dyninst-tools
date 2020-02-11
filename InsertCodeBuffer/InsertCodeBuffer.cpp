#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_image.h"
#include "BPatch_point.h"

#include "PatchMgr.h"
#include "PatchModifier.h"
#include "Point.h"
#include "Snippet.h"


using namespace Dyninst;
using namespace Dyninst::PatchAPI;

// BPatch is the top level object in Dyninst
BPatch bpatch;

// Extend PatchAPI::Snippet to emit arbitrary instructions
class PTWriteSnippet : public Dyninst::PatchAPI::Snippet {

public:
    PTWriteSnippet() {}

    // This the interface function that Dyninst will invoke during instruction code generation
    bool generate(Dyninst::PatchAPI::Point* pt, Dyninst::Buffer &buf) override {

        // Bytes for instruction PTWRITE %rax
        static unsigned char ptwrite_example[4] = {0xf3,0x0f,0xae,0xe0};

        /* What should actually happen here:
         * 1. Use the Dyninst::PatchAPI::Point to retrieve the memory access instruction
         * 2. Use InstructionAPI to examine the addressing mode of the memory access instruction
         * 3. Use Intel Xed to find the actual bytes representing the data to record
         *    For this step, we can either invoke the xed command line by passing a assembly string
         *    or include intel xec library to use its encoder interface to emit the bytes
         */

        // Dyninst::Buffer is the representation of code buffer for storing instrumentation
        // In this case, we copy the four bytes that represent "PTWRITE %rax" into the buffer.
        buf.copy(ptwrite_example, 4);
    }
};

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

int Instrument(BPatch_binaryEdit *app, BPatch_image* image) {   
    PatchMgr::Ptr patcher = Dyninst::PatchAPI::convert(app);
   
    BPatch_function* main = FindFunctionByName(image, "main");

    /* What should happen here:
     * You can either use BPatch interface or directly use PatchAPI interface
     * to identify all memory accesses.
     *
     * Here I use main's entry point as an example
     */

    std::vector<Point*> points;
    patcher->findPoints(Scope(Dyninst::PatchAPI::convert(main)), Point::FuncEntry,  back_inserter(points));

    // Construct our snippet that will emit PTWRITE instruction
    Snippet::Ptr codeBufferSnippet = PTWriteSnippet::create(new PTWriteSnippet());
    for (auto p : points) {
        p->pushBack(codeBufferSnippet);
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

    // Write the instrumented binary to disk
    app->writeFile(argv[2]);
}
