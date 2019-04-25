#include <mpi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include "sockpp/tcp_connector.h"
#include "utils.h"

int workload(const std::string &hostfile)
{
    std::ifstream ifs;
    ifs.open(hostfile);
    if (ifs.is_open()) {
        std::string str;
        std::vector<std::string> s;
        getline(ifs, str);
        str_split(str, ' ', s);
        if (s.size() != 2 || s[0] != "busyloop" || s[1] != "on") {
            usleep(1000000);
            return 0;
        }
    }
    /*
    struct stat buf;
    if (stat("/home/alexeyvmedvedev_140/vbbs_lock", &buf) != -1) {
        usleep(100000);
        return 0;
    }
    */
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

bool write_str(sockpp::stream_socket &s, const std::string &tag, const std::string &str)
{
    std::stringstream ss;
    ss << tag << ":" << str;
    std::string msg = ss.str() + "\0";
    char l = (char)(msg.length() & 0xff);
    if (s.write_n(&l, 1) != 1) 
        return false;
    if (s.write_n(msg.c_str(), msg.length()) != (int)msg.length()) 
        return false;
    return true;
}

bool write_str(sockpp::stream_socket &s, const std::string &tag, int value)
{
    std::stringstream ss;
    ss << value;
    return write_str(s, tag, ss.str());
}

int main(int argc, char **argv)
{
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::string hostfile, host, sem;
    in_port_t port = 0;
    const std::string varname = "VBBS_PARAMS";
    if (!check_environment<in_port_t>(varname, hostfile, host, port, sem, 
                                               "hostfile", "master", 13345, "vbbs_sem")) {
        std::cerr << "VBBS: environment variable " << varname << " is not set, applying defaults" << std::endl;
    }

    sockpp::socket_initializer  sockInit;
    sockpp::tcp_connector       conn;

    if (!conn.connect(sockpp::inet_address(host, port))) {
        std::cerr << "Error connecting to " << host << " port=" << port << std::endl;
        return 1;
    }
    char local[1024];
    gethostname(local, 1024);
    if (!write_str(conn, "name", local)) {
        std::cerr << "Error writing to the TCP stream: " << conn.last_error_str() << std::endl;
        MPI_Finalize();
        return 0;
    }
    while (true) {
        int prod = workload(hostfile);
        usleep(300000 * (rank % 3 + 1) + 100000 * (rank % 11));
        if (!write_str(conn, "prod", prod)) {
            std::cerr << "Error writing to the TCP stream: " << conn.last_error_str() << std::endl;
            break;
        }
    }
    MPI_Finalize();
    return 0;
}
