#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

namespace global {
    std::string semname;
    std::string hostfile;
}

#include "exceptions.h"
#include "sem.h"

namespace global {
    semaphore sem;
}

#include "node.h"
#include "func.h"
#include "utils.h"

void sighandler(int signo)
{
    (void)signo;
    if (!global::sem.inside)
        global::sem.close();
    exit(1);
}

int main(int argc, char **argv)
{
    const std::string varname = "VBBS_PARAMS";
    std::string host;
    unsigned port;
    if (!check_environment<unsigned>(varname, global::hostfile, host, port, global::semname, 
                                              "hostfile", "master", 13345, "vbbs_sem")) {
        std::cerr << "VBBS: environment variable " << varname 
                  << " is not set or is incorrect, applying defaults" << std::endl;
    }
    if (argc < 3) {
        std::cerr << "VBBS: Usage: vbbs start|stop|defunct|add|init|busyloop|sempost <N>" << std::endl;
        return 1;
    }
    if (signal(SIGTERM, sighandler) == SIG_ERR || signal(SIGINT, sighandler) == SIG_ERR ||
        signal(SIGHUP, sighandler) == SIG_ERR || signal(SIGBUS, sighandler) == SIG_ERR ||
        signal(SIGSEGV, sighandler) == SIG_ERR || signal(SIGFPE, sighandler) == SIG_ERR ||
        signal(SIGQUIT, sighandler) == SIG_ERR || signal(SIGILL, sighandler) == SIG_ERR ||
        signal(SIGABRT, sighandler) == SIG_ERR) {
        std::cerr << "VBBS: cannot setup a signal handler" << std::endl;
        return 1;
    }
    if (std::string(argv[1]) == "init") {
        global::sem.unlink();
        if (!global::sem.open(true))
            return 1;
    } else {
        if (!global::sem.open(false))
            return 1;
    }
    bool is_init_mode = (std::string(argv[1]) == "init" || std::string(argv[1]) == "sempost");
    std::string given_hostname;
    bool malformed = false;
    if (!is_init_mode) {
        if (!check_host(given_hostname, malformed)) {
            if (!malformed) {
                std::cerr << "VBBS: cannot handle batch on this host. Head hostname is " 
                          << given_hostname << std::endl;
            } else {
                std::cerr << "VBBS: hostfile is malformed" << std::endl;
            }
            return 1;
        }
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
        } else if (mode == "init") {            
            init(std::stoi(N));
        } else if (mode == "busyloop") {
            busyloop(N);
        } else if (mode == "sempost") {
            global::sem.gotit = true;
            global::sem.post();
        } else {
            std::cerr << "VBBS: unknown mode" << std::endl;
            global::sem.close();
            return 1;
        }
    }
    catch (exceptions &ex) {
        std::cerr << "EXCEPTION: " << exc2str(ex) << std::endl;
        global::sem.close();
        return 1;
    }
    catch (std::exception &ex) {
        std::cerr << "EXCEPTION: " << ex.what() << std::endl;
        global::sem.close();
        return 1;
    }
    catch (...) {
        std::cerr << "EXCEPTION" << std::endl;
        global::sem.close();
        return 1;
    }
    global::sem.close();
    return r;
}
