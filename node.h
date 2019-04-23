#pragma once

struct node {
    std::string name;
    int id;
};

struct nodelist {
    const char *name = "aaa";
    int free = 0;
    int max_id = 0;
    std::vector<node> freenodes, busynodes;
    void load() {
        freenodes.erase(freenodes.begin(), freenodes.end());
        busynodes.erase(busynodes.begin(), busynodes.end());
        free = 0;
        max_id = 0;
        std::ifstream ifs;
        ifs.open(name);
        if (!ifs.is_open()) {
            throw EX_FILE_OPEN_READ_ERROR;
        }
        while (true) {
            std::string name, state; 
            int id;
            if (!(ifs >> name >> state >> id))
                break;
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
            else if (name == "max_id") {
                if (max_id < id)
                    max_id = id;
            }
        }
        ifs.close();
    }
    void save() {
        std::ofstream ofs;
        ofs.open(name);
        if (!ofs.is_open()) {
            throw EX_FILE_OPEN_WRITE_ERROR;
        }
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
