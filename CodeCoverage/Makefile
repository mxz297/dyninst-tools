CXX = g++
CXXFLAGS = -g -Wall -std=c++11 -O2
INC = -I$(DYNINST_ROOT)/include
LIB = -L$(DYNINST_ROOT)/lib
DEP = -ldyninstAPI -lpatchAPI -lcommon -lsymtabAPI -linstructionAPI \
		-lboost_system -ltbbmalloc -ltbbmalloc_proxy

COMMON_SRC = Graph.cpp \
	  SingleBlockGraph.cpp \
	  MultiBlockGraph.cpp \
	  CoverageLocationOpt.cpp

COMMON_OBJ = $(COMMON_SRC:.cpp=.o)

all: CodeCoverage GraphTest

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INC) -o $@ $<

CodeCoverage: $(COMMON_OBJ) CodeCoverage.o
	$(CXX) $(LIB) -o $@ $^ -Wl,-rpath='$(DYNINST_ROOT)/lib' $(DEP) 

GraphTest: $(COMMON_OBJ) GraphTest.o
	$(CXX) $(LIB) -o $@ $^ -Wl,-rpath='$(DYNINST_ROOT)/lib' $(DEP) 

clean:
	rm -f CodeCoverage GraphTest *.o
