MPICXX ?= mpicxx

all: vbbs vbbs_client vbbs_server

vbbs: Makefile exceptions.h  func.h  node.h  sem.h

vbbs: vbbs.cpp
	$(CXX) -O2 -Wall -Wextra -pedantic -std=c++11 -o $@ $< -lpthread

vbbs_client: vbbs_client.cpp
	$(MPICXX) -I. -L. -O2 -Wall -Wextra -std=c++11 -o $@ $< -lpthread -lsockpp

vbbs_server: vbbs_server.cpp
	$(CXX) -I. -L. -O0 -g -Wall -Wextra  -std=c++11 -o $@ $< -lpthread -lsockpp

clean:
	rm -f vbbs vbbs_client vbbs_server

install:
	cp libsock* ~/lib
	cp libsock* ~/_scratch/lib
	cp vbbs vbbs_client vbbs_server ~/bin
	cp vbbs vbbs_client vbbs_server ~/_scratch/bin/
	cp vbbs vbbs_client vbbs_server ~/_scratch/vbbs/

