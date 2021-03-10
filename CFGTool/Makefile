CFLAGS = -g -O2 -std=c++11
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib 
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -lelf -ldynDwarf -ldw -lelf -ldynElf -lcommon -lboost_system


CFGTool: CFGTool.cpp
	g++ $(CFLAGS) $(INC) $(LIB) CFGTool.cpp -o CFGTool $(DEP)  -Wl,-rpath $(DYNINST_ROOT)/lib

clean:
	rm -f CFGTool
