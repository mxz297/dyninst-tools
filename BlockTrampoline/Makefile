all: BlockTrampoline

BlockTrampoline: BlockTrampoline.cpp
	g++ -g -o BlockTrampoline -O2 -std=c++11 BlockTrampoline.cpp \
		-I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib \
		-ldyninstAPI -lboost_system -ltbbmalloc -ltbbmalloc_proxy \
		-Wl,-rpath='$(DYNINST_ROOT)/lib'

clean:
	rm -f BlockTrampoline
