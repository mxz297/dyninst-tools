DYNINST_ROOT=/home/xm13/tmp/ShadowGuard/thirdparty/dyninst-10.1.0/install
#DYNINST_ROOT=/home/xm13/projects/liteCFI/thirdparty/dyninst-10.1.0/install

all: CallGraph FuncSort libcg.so

CallGraph: CallGraph.cpp
	g++ -g -o CallGraph -O2 -std=c++11 CallGraph.cpp \
		-I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib \
		-ldyninstAPI -lboost_system \
		-Wl,-rpath='$(DYNINST_ROOT)/lib'

FuncSort: FuncSort.cpp
	g++ -g -o FuncSort -O2 -std=c++11 FuncSort.cpp \
		-I$(DYNINST_ROOT)/include \
		-L$(DYNINST_ROOT)/lib \
		-ldyninstAPI -lcommon -lboost_system \
		-Wl,-rpath='$(DYNINST_ROOT)/lib'

libcg.so: libcg.cpp
	g++ -g -o libcg.so -shared -fPIC -O2 libcg.cpp -std=c++11

clean:
	rm -f CallGraph FuncSort libcg.so
