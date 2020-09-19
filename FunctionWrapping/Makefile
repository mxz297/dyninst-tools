DYNINST_ROOT=/home/xm13/dyninstapi/install
DEP_MEMORY = -ldyninstAPI -lparseAPI -linstructionAPI -lcommon -lboost_system

all: FuncWrap libwrap.so test 

FuncWrap: FuncWrap.cpp
	g++ -O2 -g -I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib $(DEP_MEMORY) \
		-Wl,-rpath='$(DYNINST_ROOT)/lib' \
		-o FuncWrap FuncWrap.cpp

libwrap.so: libwrap.c
	gcc -fPIC -shared -o libwrap.so libwrap.c

test: test.c
	gcc -o test test.c

clean:
	rm -f FuncWrap test libwrap.so
