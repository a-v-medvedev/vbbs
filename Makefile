MPICXX ?= mpicxx

# FIXME __USE_BSD is a fix for gcc 9.1 on Lom2
CXX_FLAGS += -D__USE_BSD -O0 -g -DWITH_DEBUG -Isockpp.bin/include -Lsockpp.bin/lib64
#CXX_FLAGS += -D__USE_BSD -O2 -DWITH_DEBUG -Isockpp.bin/include -Lsockpp.bin/lib64

all: vbbs vbbs_client vbbs_server

vbbs: Makefile exceptions.h  func.h  node.h  sem.h

vbbs: vbbs.cpp
	$(CXX) -Wall -Wextra -pedantic -std=c++11 $(CXX_FLAGS) -o $@ $< -lpthread

vbbs_client: vbbs_client.cpp
	$(MPICXX) -I. -L. -Wall -Wextra -std=c++11 $(CXX_FLAGS) -o $@ $< -lpthread sockpp.bin/lib64/libsockpp.a

vbbs_server: vbbs_server.cpp
	$(CXX) -I. -L. -Wall -Wextra  -std=c++11 $(CXX_FLAGS) -o $@ $< -lpthread sockpp.bin/lib64/libsockpp.a

clean:
	rm -f vbbs vbbs_client vbbs_server

install:
	cp libsock* ~/lib
	cp libsock* ~/_scratch/lib
	cp vbbs vbbs_client vbbs_server ~/bin
	cp vbbs vbbs_client vbbs_server ~/_scratch/bin/
	cp vbbs vbbs_client vbbs_server ~/_scratch/vbbs/

