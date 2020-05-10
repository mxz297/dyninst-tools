DYNINST_ROOT = /home/xm13/dyninst-pp/install
CFLAGS = -g -O2 -std=c++11
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib 
DEP = -lparseAPI -linstructionAPI -lsymtabAPI -lelf -ldynDwarf -ldw -lelf -ldynElf -lcommon -lboost_system


CodeDump: CodeDump.cpp
	g++ $(CFLAGS) $(INC) $(LIB) CodeDump.cpp -o CodeDump $(DEP)  -Wl,-rpath $(DYNINST_ROOT)/lib

clean:
	rm -f CodeDump
