vbbs: Makefile exceptions.h  func.h  node.h  sem.h

vbbs: vbbs.cpp
	$(CXX) -O2 -Wall -Wextra -pedantic -std=c++11 -o $@ $< -lpthread

clean:
	rm -f vbbs vbbs.o
