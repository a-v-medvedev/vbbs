#pragma once

struct node {
    std::string name;
    int id;
};

struct nodelist {
    int free = 0;
    int max_id = 0;
    std::vector<node> freenodes, busynodes;
    std::string hostname;
    static void init(int N) {
        std::ofstream ofs;
        ofs.open(global::hostfile);
        if (!ofs.is_open()) {
            throw EX_FILE_OPEN_WRITE_ERROR;
        }
        char local[1024];
        gethostname(local, 1024);
        ofs << "head " << local << " " << 0 << std::endl;
        ofs << "max_id - " << N << std::endl;
        ofs.close();
    }
    static bool check_host(std::string &hostname, bool &malformed) {
        int max_id = -1;
        bool resolved_as_local = false;
        std::ifstream ifs;
        ifs.open(global::hostfile);
        if (!ifs.is_open()) {
            throw EX_FILE_OPEN_READ_ERROR;
        }
        while (true) {
            std::string name, state; 
            int id;
            if (!(ifs >> name >> state >> id))
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
            }
        }
        ifs.close();
        malformed = (!resolved_as_local && hostname.size() == 0) || max_id < 0;
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
            int id;
            if (!(ifs >> name >> state >> id))
                break;
            if (name == "max_id") {
                if (max_id < id)
                    max_id = id;
                continue;
            } else if (name == "head") {
                hostname = state;
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
        ofs << "head " << hostname << " " << 0 << std::endl;
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
        ofs.close(); 
    }
};
