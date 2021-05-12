
CFLAGS = -g -O2 -std=c++11
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib 
DEP = -lsymtabAPI


CatchBlocks: CatchBlocks.cpp
	g++ $(CFLAGS) $(INC) $(LIB) CatchBlocks.cpp -o CatchBlocks $(DEP)  -Wl,-rpath $(DYNINST_ROOT)/lib

clean:
	rm -f CatchBlocks
