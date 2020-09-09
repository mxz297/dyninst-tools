#include "CodeSource.h"
#include "CodeObject.h"
#include "CFG.h"
#include "Instruction.h"

#include "Absloc.h"
#include "AbslocInterface.h"
#include "SymEval.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::DataflowAPI;
std::map<Address, string> output;

static char *me;
static int return_code = 0;

void usage(bool not_ok = true, string mssg = "")
{
  if (not_ok) {
      if (mssg.size () > 0)
        cerr << mssg << endl;
      cerr << "Usage: " << me << " -addr A <BINARY>\n"
           << "Print symbolic expression for instruction at address A\n";
      exit (EXIT_FAILURE);
    }
}
  

void PrintSymbolicExpression(Function* f, Block *b, Address a, Instruction& i) {    
    AssignmentConverter ac(true, false);
    vector<Assignment::Ptr> assignments;
    ac.convert(i, a, f, b, assignments);
    printf("Instruction %s at address %lx has %u assignments, opcode %d\n",
            i.format().c_str(), a, assignments.size(), i.getOperation().getID());

    for (auto & a : assignments) {
        printf("\t Expanding assignment %s\n", a->format().c_str());
        pair<AST::Ptr, bool> expandRet = SymEval::expand(a, false);
        if (expandRet.second && expandRet.first) {
            printf("\t\t%s\n", expandRet.first->format().c_str());
        } else {
            printf("\t\tFailed to get symbolic expression\n");
        }
    }
}

int main(int argc, char **argv) {
    string executable;
    me = argv[0];
    Address addr = 0;
    for (int argn = 1; argn < argc; argn++) {
        if (strcmp (argv[argn], "-addr") == 0) {
            argn++;
            char* endptr;
            addr = strtoul(argv[argn], &endptr, 16);
        }
        else if (strcmp (argv[argn], "-h") == 0) {
            usage(true);
        }
        else executable = argv[argn];
    }
    usage(executable.size() == 0, "missing BINARY");

    SymtabCodeSource *sts;
    CodeObject *co;

    sts = new SymtabCodeSource((char*)executable.c_str());
    co = new CodeObject(sts);
    co->parse();   
    // Iterate all functions
    for (auto f : co->funcs()) {
        // Iterate all blocks in a function
        for (auto b : f->blocks()) {
            // Get all instrucitons in a block
            Block::Insns insns;
            b->getInsns(insns);
            for (auto &iit : insns) {
                if (iit.first == addr) {
                    PrintSymbolicExpression(f, b, addr, iit.second);
                    return 0;
                }
            }
        }
    }
    fprintf(stderr, "Cannot find instruction at address %lx\n", addr);
    return EXIT_FAILURE;
}
