CFLAGS = -g -O2 -std=c++11
INC = -I$(DYNINST_ROOT)/include -I$(BOOST_ROOT)/include -I$(TBB_INCLUDE)
LIB = -L$(DYNINST_ROOT)/lib -L$(BOOST_ROOT)/lib -L$(ELFUTIL_LIB)
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -lelf -ldynDwarf -ldw -lelf -ldynElf -lcommon -lboost_system

CFGConsistency: CFGConsistency.cpp
	g++ $(CFLAGS) $(INC) $(LIB) CFGConsistency.cpp -o CFGConsistency $(DEP)

clean:
	rm -f CFGConsistency

