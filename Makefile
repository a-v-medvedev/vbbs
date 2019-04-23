all: vbbs vbbs_client vbbs_server

vbbs: Makefile exceptions.h  func.h  node.h  sem.h

vbbs: vbbs.cpp
	$(CXX) -O2 -Wall -Wextra -pedantic -std=c++11 -o $@ $< -lpthread

vbbs_client: vbbs_client.cpp
	$(CXX) -I. -L. -O2 -Wall -Wextra -std=c++11 -o $@ $< -lpthread -lsockpp

vbbs_server: vbbs_server.cpp
	$(CXX) -I. -L. -O2 -Wall -Wextra  -std=c++11 -o $@ $< -lpthread -lsockpp

clean:
	rm -f vbbs vbbs_client vbbs_server
