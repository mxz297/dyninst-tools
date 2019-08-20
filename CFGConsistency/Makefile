CFLAGS = -g -O2 -std=c++11
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib -L$(DYNINST_ROOT)/../elfutils/install/lib
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -lelf -ldynDwarf -ldw -lelf -ldynElf -lcommon -lboost_system

CFGConsistency: CFGConsistency.cpp
	g++ $(CFLAGS) $(INC) $(LIB) -Wl,-rpath='$(DYNINST_ROOT)/lib:/home/xmeng/dyninstapi/elfutils/install/lib' CFGConsistency.cpp -o CFGConsistency $(DEP)

clean:
	rm -f CFGConsistency

