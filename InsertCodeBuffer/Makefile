DYNINST_ROOT=/home/xm13/dyninstapi/install
DEP = -ldyninstAPI -lpatchAPI -linstructionAPI -lcommon -lboost_system

InsertCodeBuffer: InsertCodeBuffer.cpp
	g++ -O2 -g -I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib $(DEP) \
		-Wl,-rpath='$(DYNINST_ROOT)/lib' \
		-o InsertCodeBuffer InsertCodeBuffer.cpp

clean:
	rm -f InsertCodeBuffer InsertCodeBuffer.ptwrite
