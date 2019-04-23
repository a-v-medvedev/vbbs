#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "exceptions.h"
#include "sem.h"

semaphore sem;

#include "node.h"
#include "func.h"

void sighandler(int signo)
{
    (void)signo;
    if (!sem.inside)
        sem.close();
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cerr << "VBBS: Usage: vbbs start|stop|defunct|add <N>" << std::endl;
        return 1;
    }
    if (signal(SIGTERM, sighandler) == SIG_ERR || signal(SIGINT, sighandler) == SIG_ERR) {
        std::cerr << "VBBS: cannot setup a signal handler" << std::endl;
        return 1;
    }
    if (!sem.open()) {
        return 1;
    }
    int r = 0;
    try {
        std::string mode(argv[1]);
        std::string N(argv[2]);
        if (mode == "start") {
            r = start(std::stoi(N));
        } else if (mode == "stop") {
            r = stop(std::stoi(N));
        } else if (mode == "defunct") {
            defunct(N);
        } else if (mode == "add") {
            add(N);
        } else {
            std::cerr << "VBBS: unknown mode" << std::endl;
            sem.close();
            return 1;
        }
    }
    catch (exceptions &ex) {
        std::cerr << "EXCEPTION: " << int(ex) << std::endl;
        sem.close();
        return 1;
    }
    catch (...) {
        std::cerr << "EXCEPTION" << std::endl;
        sem.close();
        return 1;
    }
    sem.close();
    return r;
}
