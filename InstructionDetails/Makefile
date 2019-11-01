DYNINST_ROOT = /home/xm13/projects/liteCFI/thirdparty/dyninst-10.1.0/install
CFLAGS = -g -O2 -std=c++11
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib 
DEP = -linstructionAPI -lelf -ldynDwarf -ldw -lelf -ldynElf -lcommon -lboost_system


Instruction: Instruction.cpp
	g++ $(CFLAGS) $(INC) $(LIB) Instruction.cpp -o Instruction $(DEP)

clean:
	rm -f Instruction
