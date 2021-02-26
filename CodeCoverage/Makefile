all: CodeCoverage

CodeCoverage: CodeCoverage.cpp
	g++ -g -o CodeCoverage -O2 -std=c++11 CodeCoverage.cpp \
		-I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib \
		-ldyninstAPI -lpatchAPI -lcommon -linstructionAPI \
		-lboost_system -ltbbmalloc -ltbbmalloc_proxy \
		-Wl,-rpath='$(DYNINST_ROOT)/lib'

clean:
	rm -f CodeCoverage
