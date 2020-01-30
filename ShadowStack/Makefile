DYNINST_ROOT=/home/xm13/dyninstapi/install
DEP_SIMPLE = -ldyninstAPI -lboost_system
DEP_MEMORY = -ldyninstAPI -lparseAPI -linstructionAPI -lcommon -lboost_system


all: MemoryAccessShadowStack SimpleShadowStack libshadowstack.so test

MemoryAccessShadowStack: MemoryAccessShadowStack.cpp
	g++ -O2 -g -I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib $(DEP_MEMORY) \
		-Wl,-rpath='$(DYNINST_ROOT)/lib' \
		-o MemoryAccessShadowStack MemoryAccessShadowStack.cpp

SimpleShadowStack: SimpleShadowStack.cpp
	g++ -O2 -g -I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib $(DEP_SIMPLE) \
		-Wl,-rpath='$(DYNINST_ROOT)/lib' \
		-o SimpleShadowStack SimpleShadowStack.cpp

libshadowstack.so: shadowstack.c
	gcc -fPIC -shared -o libshadowstack.so shadowstack.c

test: test.c
	gcc -o test test.c

clean:
	rm -f SimpleShadowStack test test.inst libshadowstack.so
