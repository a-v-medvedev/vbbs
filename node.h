#pragma once

#include <stdexcept>
#include "utils.h"
#include <algorithm>
#include <unistd.h>

struct node {
    std::string name;
    int id;
};

bool operator<(const node &lhs, const node &rhs) {
    return lhs.name < rhs.name;
}

static bool parse_next_line(std::ifstream &ifs, 
                            std::string &name, std::string &state, int &id, 
                            bool &malformed) {
    std::string str;
    std::vector<std::string> parts;
    if (!getline(ifs, str))
        return false;
    str_split(str, ' ', parts);
    if (parts.size() > 0) {
        name = parts[0];
    }
    if (parts.size() > 1) {
        state = parts[1];
    }
    if (parts.size() > 2) {
        try {
            id = std::stoi(parts[2]);
        }
        catch(std::invalid_argument &) { malformed = true; }
        catch(std::out_of_range &) { malformed = true; }
    }
    if (parts.size() > 3)
        malformed = true;
    return true;
}

struct nodelist {
    int free = 0;
    int max_id = 0;
    int slurm_id = 0;
    std::vector<node> freenodes, busynodes;
    std::string hostname;
    std::string busyloop;
    static void init(int N, int slurm_id = 0) {
        std::ofstream ofs;
        ofs.open(global::hostfile);
        if (!ofs.is_open()) {
            throw EX_FILE_OPEN_WRITE_ERROR;
        }
        ofs << "busyloop " << "on" << std::endl;
        char local[1024];
        gethostname(local, 1024);
        ofs << "head " << local << std::endl;
        ofs << "max_id - " << N << std::endl;
        ofs << "slurm_id - " << slurm_id << std::endl; 
        ofs.flush();
        usleep(10000);
        ofs.close();
        usleep(10000);
    }
    static bool check_host(std::string &hostname, bool &malformed) {
        int max_id = -1, slurm_id = -1;
        bool resolved_as_local = false;
        bool has_busyloop = false;
        std::ifstream ifs;
        ifs.open(global::hostfile);
        if (!ifs.is_open()) {
            throw EX_FILE_OPEN_READ_ERROR;
        }
        while (true) {
            std::string name, state;
            int id = -1;
            if (!parse_next_line(ifs, name, state, id, malformed))
                break;
            if (name == "max_id") {
                if (max_id < id)
                    max_id = id;
                continue;
            } else if (name == "head") {
                char local[1024];
                hostname = state;
                gethostname(local, 1024);
                resolved_as_local = (hostname == local);
            } else if (name == "busyloop") {
                has_busyloop = (state == "on" || state == "off");
            }
        }
        ifs.close();
        malformed = malformed || (!resolved_as_local || hostname.size() == 0);
        malformed = malformed || (max_id < 0);
        malformed = malformed || (slurm_id < 0);
        malformed = malformed || (!has_busyloop);
        return resolved_as_local; 
    }
    void load() {
        freenodes.erase(freenodes.begin(), freenodes.end());
        busynodes.erase(busynodes.begin(), busynodes.end());
        free = 0;
        max_id = 0;
        std::ifstream ifs;
        ifs.open(global::hostfile);
        if (!ifs.is_open()) {
            throw EX_FILE_OPEN_READ_ERROR;
        }
        while (true) {
            std::string name, state; 
            int id = 0;
            bool malformed;
            if (!parse_next_line(ifs, name, state, id, malformed))
                break;
            if (name == "max_id") {
                if (max_id < id)
                    max_id = id;
                continue;
            } else if (name == "slurm_id") {
                slurm_id = id;
                continue;
            } else if (name == "head") {
                hostname = state;
                continue;
            } else if (name == "busyloop") {
                busyloop = state;
                continue;
            }

            if (state == "free") {
                freenodes.push_back(node { name, id } );
                free++;
            }
            else if (state == "busy") {
                busynodes.push_back(node { name, id } );
                if (max_id < id)
                    max_id = id;
            } 
            else if (state == "defunct") {
                freenodes.push_back(node { name, -1 } );
            } 
        }
        ifs.close();
    }

    void save() {
        std::ofstream ofs;
        ofs.open(global::hostfile);
        if (!ofs.is_open()) {
            throw EX_FILE_OPEN_WRITE_ERROR;
        }
        std::sort(freenodes.begin(), freenodes.end());
        std::sort(busynodes.begin(), busynodes.end());
        ofs << "busyloop " << busyloop << std::endl;
        ofs << "head " << hostname << std::endl;
        for (auto &n : freenodes) {
            std::string state = (n.id == -1 ? "defunct" : "free");
            if (!(ofs << n.name << " " << state << " " << 0 << std::endl))
                throw EX_FILE_OPEN_WRITE_ERROR;
        }
        for (auto &n : busynodes) {
            if (!(ofs << n.name << " " << "busy" << " " << n.id << std::endl))
                throw EX_FILE_OPEN_WRITE_ERROR;
        }
        ofs << "max_id - " << max_id << std::endl;
        ofs << "slurm_id - " << slurm_id << std::endl;
        ofs.flush();
        usleep(10000);
        ofs.close(); 
        usleep(10000);
    }
};
