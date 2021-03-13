all: CodeCoverage GraphTest

CodeCoverage: CodeCoverage.cpp Graph.cpp SingleBlockGraph.cpp MultiBlockGraph.cpp CoverageLocationOpt.cpp
	g++ -g -o CodeCoverage -O2 -std=c++11 \
	    CodeCoverage.cpp Graph.cpp SingleBlockGraph.cpp MultiBlockGraph.cpp CoverageLocationOpt.cpp \
		-I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib \
		-ldyninstAPI -lpatchAPI -lcommon -lsymtabAPI -linstructionAPI \
		-lboost_system -ltbbmalloc -ltbbmalloc_proxy \
		-Wl,-rpath='$(DYNINST_ROOT)/lib'

GraphTest: GraphTest.cpp Graph.cpp SingleBlockGraph.cpp MultiBlockGraph.cpp CoverageLocationOpt.cpp
	g++ -g -o GraphTest -O2 -std=c++11 \
		GraphTest.cpp Graph.cpp SingleBlockGraph.cpp MultiBlockGraph.cpp CoverageLocationOpt.cpp \
		-I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib \
		-lpatchAPI \
		-Wl,-rpath='$(DYNINST_ROOT)/lib'


clean:
	rm -f CodeCoverage GraphTest
