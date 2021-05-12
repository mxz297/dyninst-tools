CXX = g++
CXXFLAGS = -g -Wall -std=c++11 -O2 -fopenmp
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib
DEP = -ldyninstAPI -lpatchAPI -lcommon -lsymtabAPI -linstructionAPI \
		-lparseAPI -lboost_system -ltbb -ltbbmalloc -ltbbmalloc_proxy -fopenmp		

COVERAGE_SRC = CodeCoverage.cpp \
	CoverageSnippet.cpp \
	LoopCloneOptimizer.cpp

COVERAGE_OBJ = $(COVERAGE_SRC:.cpp=.o)

COMMON_SRC = Graph.cpp \
	  SingleBlockGraph.cpp \
	  MultiBlockGraph.cpp \
	  CoverageLocationOpt.cpp

COMMON_OBJ = $(COMMON_SRC:.cpp=.o)

all: CodeCoverage GraphTest

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INC) -o $@ $<

CodeCoverage: $(COMMON_OBJ) $(COVERAGE_OBJ)
	$(CXX) $(LIB) -o $@ $^ -Wl,-rpath='$(DYNINST_ROOT)/lib' $(DEP)

GraphTest: $(COMMON_OBJ) GraphTest.o
	$(CXX) $(LIB) -o $@ $^ -Wl,-rpath='$(DYNINST_ROOT)/lib' $(DEP)

clean:
	rm -f CodeCoverage GraphTest *.o
