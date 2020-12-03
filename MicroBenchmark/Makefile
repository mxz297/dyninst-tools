CFLAGS = -g -O2 -std=c++11

#DYNINST_INSTALL=/home/xm13/projects/liteCFI/thirdparty/dyninst-10.1.0/install
#DYNINST_INSTALL=/home/xm13/dyninst-pp/install
DYNINST_INSTALL=/home/xm13/dyninstapi/install

all: micro-parse micro-symtab

micro-parse: micro-parse.cpp
	g++ $(CFLAGS) -I$(DYNINST_INSTALL)/include \
		-L$(DYNINST_INSTALL)/lib \
		-Wl,-rpath='$(DYNINST_INSTALL)/lib' \
		micro-parse.cpp -o micro-parse -lparseAPI -lsymtabAPI -lboost_system -ltbb -ltbbmalloc -ltbbmalloc_proxy

micro-symtab: micro-symtab.cpp
	g++ $(CFLAGS) -I$(DYNINST_INSTALL)/include \
		-L$(DYNINST_INSTALL)/lib \
		-Wl,-rpath='$(DYNINST_INSTALL)/lib' \
		micro-symtab.cpp -o micro-symtab -lsymtabAPI -lboost_system

clean:
	rm -f micro-parse micro-symtab
