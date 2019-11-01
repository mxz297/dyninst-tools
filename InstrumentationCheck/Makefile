DYNINST_ROOT = /home/xm13/projects/liteCFI/thirdparty/dyninst-10.1.0/install
CFLAGS = -g -O2 -std=c++11
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -lelf -ldynDwarf -ldw -lelf -ldynElf -lcommon -lboost_system


InstrumentationCheck: InstrumentationCheck.cpp
	g++ $(CFLAGS) $(INC) $(LIB) InstrumentationCheck.cpp -o InstrumentationCheck $(DEP)

clean:
	rm -f InstrumentationCheck
