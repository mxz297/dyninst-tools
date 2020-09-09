DYNINST_ROOT = /home/xmeng/dyninstapi/install
CFLAGS = -g -O2 -std=c++11
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib 
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -lelf -ldynDwarf -ldw -lelf -ldynElf -lcommon -lboost_system


SymbolicExpression: SymbolicExpression.cpp
	g++ $(CFLAGS) $(INC) $(LIB) SymbolicExpression.cpp -o SymbolicExpression $(DEP)  -Wl,-rpath $(DYNINST_ROOT)/lib

clean:
	rm -f SymbolicExpression
