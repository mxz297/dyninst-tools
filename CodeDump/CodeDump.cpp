#include "CodeSource.h"
#include "CodeObject.h"
#include "CFG.h"
#include "Instruction.h"
using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;
using namespace Dyninst::InstructionAPI;
std::map<Address, string> output;

static char *me;
static int return_code = 0;
static const char *x8664_registers [] = 
  { "rax", "rbx", "rcx", "rdx", "rbp", "rsp", "rdi", "rsi",  };
static Dyninst::MachRegister  x8664_mach_registers [] = 
  { x86_64::rax, x86_64::rbx, x86_64::rcx, x86_64::rdx, x86_64::rbp, x86_64::rsp, x86_64::rdi, x86_64::rsi };


void usage(bool not_ok = true, string mssg = "")
{
  if (not_ok) {
      if (mssg.size () > 0)
        cerr << mssg << endl;
      cerr << "Usage: " << me << " [-verobse] [-brief] [-register REGISTER] <BINARY>\nDump the discovered code from the BINARY\n-register Only dump instructions matching REGISTER\n-verbose Dump all instructions\n-brief (default) Parse binary but do not dump instructions\n";
      exit (EXIT_FAILURE);
    }
}
  

int main(int argc, char **argv) {
    string base_register = "rbx";
    Dyninst::MachRegister base_mach_register;
    string executable;
    enum { match_register, brief, verbose } output_type = brief;

    me = argv[0];
    for (int argn = 1; argn < argc; argn++) {
        if (strcmp (argv[argn], "-brief") == 0)
            output_type = brief;
	else if (strcmp (argv[argn], "-verbose") == 0)
            output_type = verbose;
        else if (strcmp (argv[argn], "-register") == 0) {
            usage (++argn >= argc, "-register is missing a REGISTER");
            base_register = argv[argn];
	    output_type = match_register;
        }
        else if (strcmp (argv[argn], "-h") == 0) {
            usage(true);
        }
	else executable = argv[argn];
    }
    usage(executable.size() == 0, "missing BINARY");

    int r;
    for (r = 0; r < 8; r++)
      if (strcmp (x8664_registers[r], base_register.c_str()) == 0)
	  break;
    usage (r == 8, "Unknown x86 register, expected one of rax, rbx, rcx, rdx, rbp, rsp, rdi, rsi");
    base_mach_register = x8664_mach_registers[r];

    SymtabCodeSource *sts;
    CodeObject *co;

    sts = new SymtabCodeSource((char*)executable.c_str());
    co = new CodeObject(sts);
    co->parse();   
    int count = 0;
    // Iterate all functions
    for (auto f : co->funcs()) {
        // Iterate all blocks in a function
        for (auto b : f->blocks()) {
            // Get all instrucitons in a block
            Block::Insns insns;
            b->getInsns(insns);
	    if (!return_code && insns.size() == 0)
	      // Consider success as finding insns for all blocks
	        return_code == 1;
	    if (output_type == brief) 
	      // For -brief assume fetching insns is good enough
	        continue;
	    
            for (auto insn_iter : insns) {
                Instruction & i = insn_iter.second;
                std::set<Dyninst::InstructionAPI::RegisterAST::Ptr> read;
                i.getReadSet(read);  
                bool read_register = false;
                for (auto const& r : read) {
		  if (r->getID().getBaseRegister() == base_mach_register) {
                      read_register = true;
                    }
                }
                if ((output_type == match_register && read_register) 
		    || (output_type == verbose)) {
                    output[insn_iter.first] = i.format();
                }
            }
        }
    }
    if (output_type != brief) {
        if (output_type == match_register && output.size() == 0)
	    cout << "No instruction references to register " << base_register << endl;
     
        for (auto it : output) {
	    printf("%lx %s\n", it.first, it.second.c_str());
       }
    }

    exit (return_code);
}
