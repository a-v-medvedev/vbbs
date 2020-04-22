#include <mpi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include "sockpp/tcp_connector.h"
#include "utils.h"


bool check_workload_mode(const std::string &hostfile) 
{
    bool no_workload = false;
    std::ifstream ifs;
    ifs.open(hostfile);
    if (ifs.is_open()) {
        std::string str;
        std::vector<std::string> s;
        getline(ifs, str);
        str_split(str, ' ', s);
        if (s.size() != 2 || s[0] != "busyloop" || s[1] != "on") {
            no_workload = true;
        }
        ifs.close();
    } else {
        no_workload = true;
    }
    return !no_workload;
}

int workload()
{
    MPI_Request *reqs = nullptr;
    int num_requests = 0; 
    int stat[10] = { 0, };
    int total_tests = 0;
    int successful_tests = 0;
    static const int SIZE = 7;
    double a[SIZE][SIZE], b[SIZE][SIZE], c[SIZE][SIZE];
    for (int i = 0; i < SIZE; i++) {
        for (int j=0; j< SIZE; j++) {
            a[i][j] = b[i][j] = c[i][j] = 1.;
        }
    }
    int R = (50000000 / (2 * SIZE*SIZE)) + 1;
    double timings[3];
    int warmup = 100;
    for (int k = 0; k < 3 + warmup; k++) {
        double t1 = MPI_Wtime();
        double tover = 0;
        for (int repeat = 0, cnt = 99999999; repeat < R; repeat++) {
            if (--cnt == 0) {
                double ot1 = MPI_Wtime();
                if (reqs && num_requests) {
                    for (int r = 0; r < num_requests; r++) {
                        if (!stat[r]) {
                            total_tests++;
                            MPI_Test(&reqs[r], &stat[r], MPI_STATUS_IGNORE);
                            if (stat[r]) {
                                successful_tests++;
                            }
                        }
                    }
                }
                double ot2 = MPI_Wtime();
                tover += (ot2 - ot1);
            }
            for (int i = 0; i < SIZE; i++) {
                 for (int j = 0; j < SIZE; j++) {
                      for (int k = 0; k < SIZE; k++) {
                           c[i][j] += a[i][k] * b[k][j] + repeat*repeat;
                      }
                 }
            }
        }
        double t2 = MPI_Wtime();
        if (k >= warmup)
            timings[k-warmup] = t2 - t1;
    }
    double tmedian = std::min(timings[0], timings[1]);
    if (tmedian < timings[2])
        tmedian = std::min(std::max(timings[0], timings[1]), timings[2]);
    int ncalcs = (int)((double)R / (tmedian * 1.0e5) + 0.99);
    return ncalcs;
}


namespace sys 
{
    std::string myhostname() 
    {
        char local[1024];
        gethostname(local, 1024);
        local[1024-1] = 0;
        return std::string(local);
    }
}

namespace comm 
{
    std::shared_ptr<sockpp::tcp_connector> get_new_sock()
    {
        return std::make_shared<sockpp::tcp_connector>();
    }

    bool connect(std::shared_ptr<sockpp::tcp_connector> &sock, 
                 const std::string &host, in_port_t port) {
        bool connected = false;
        for (int i = 0; i < 5; i++) {
            std::cout << "vbbs_client: connecting to " << host << " " << port << std::endl;
            if (!sock->connect(sockpp::inet_address(host, port))) {
                continue;
            }
            connected = true;
            break;
        }
        if (!connected) {
            std::cerr << "vbbs_client: error connecting to " << host << " port="
                      << port << std::endl;
            return false;
        }
        return true;
    }

    bool write_str(std::shared_ptr<sockpp::tcp_connector> &sock, const std::string &tag, 
                   const std::string &str)
    {
        std::stringstream ss;
        ss << tag << ":" << str;
        std::string msg = ss.str() + "\0";
        char l = (char)(msg.length() & 0xff);
        if (sock->write_n(&l, 1) != 1) 
            return false;
        if (sock->write_n(msg.c_str(), msg.length()) != (int)msg.length()) 
            return false;
        return true;
    }

    bool write_str(std::shared_ptr<sockpp::tcp_connector> &sock, const std::string &tag, 
                   int value)
    {
        std::stringstream ss;
        ss << value;
        return write_str(sock, tag, ss.str());
    }

    std::string lasterror(std::shared_ptr<sockpp::tcp_connector> &sock)
    {
        return sock->last_error_str();
    }
}

int main(int argc, char **argv)
{
    int rank, per_node_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::string hostfile, host, sem;
    in_port_t port = 0;
    const std::string varname = "VBBS_PARAMS";
    if (!check_environment<in_port_t>(varname, hostfile, host, port, sem, 
                                               "hostfile", "master", 13345, "vbbs_sem")) {
        std::cerr << "vbbs_client: environment variable " << varname 
                  << " is not set, applying defaults" << std::endl;
    }
    MPI_Comm per_node_comm;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &per_node_comm);
    MPI_Comm_rank(per_node_comm, &per_node_rank);
    auto sock = comm::get_new_sock();
    if (per_node_rank == 0) {
        if (!comm::connect(sock, host, port)) {
            return 1;
        }
        auto hostname = sys::myhostname();
        std::cout << "vbbs_client: writing: name=" << hostname << std::endl;
        if (!comm::write_str(sock, "name", hostname)) {
            std::cerr << "vbbs_client: error writing to the TCP stream: " 
                      << comm::lasterror(sock) << std::endl;
            return 0;
        }
    }
    int cnt = 0;
    while (true) {
        int prod = 0;
        if (!check_workload_mode(hostfile)) {
            usleep(1000000);
        } else {
            prod = workload();
        }
        // TODO: add combining of prod results inside a node 
        // FIXME blocking MPI_Reduce results in waiting: each rank has its own delay
        //MPI_Reduce(rank == 0 ? MPI_IN_PLACE : &prod, &prod, 1, MPI_INT, MPI_MIN, 0, per_node_comm);
        ++cnt;
        cnt = (cnt % 17);
        usleep(30 * ((rank + cnt) % 3 + 1) + 10 * ((rank + cnt) % 11));
        if (per_node_rank == 0) {
            if (!comm::write_str(sock, "prod", prod)) {
                std::cerr << "vbbs_client: error writing to the TCP stream: " 
                          << comm::lasterror(sock) << std::endl;
                return 0;
            }
        }
    }
    // Unreachable:
    //MPI_Finalize();
    return 0;
}
