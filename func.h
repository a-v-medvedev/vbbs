#pragma once

#include <unistd.h>
#include <regex>
#include <assert.h>
#include <algorithm>


bool check_host(std::string &hostname, bool &malformed)
{
    global::sem.wait();
    auto r = nodelist::check_host(hostname, malformed);
    global::sem.post();
    return r;
}

void init(int N)
{
    global::sem.wait();
    nodelist::init(N);
    global::sem.post();
}

void slurm_init()
{
    using namespace utils::str;
    auto slurm_id = std::stoi(getenv("SLURM_JOBID", "FATAL: environment variable SLURM_JOBID must be defined"));
    auto envnodes = getenv("SLURM_NODELIST", "FATAL: environment variable SLURM_NODELIST must be defined");
    envnodes = std::regex_replace(envnodes, std::regex(",([^0-9])"), " $1");
    std::vector<std::string> slurm_nodes;
    for (const auto &item : split(envnodes, ' ')) {
       	if (!contains(item, '[')) {
			slurm_nodes.push_back(item);
	   	} else {
			auto B = split(item, '[');
            assert(B.size() == 2);
			const auto &main = B[0];
            assert(B[1].back() == ']');
            B[1].pop_back(); 
			const auto &variable = B[1];
			for (const auto &v : split(variable, ',')) {
				if (!contains(v, '-')) {
					slurm_nodes.push_back(main + v);
				} else {
					const auto limits = split(v, '-');
                    assert(limits.size() == 2);
                    assert(limits[0].length() == limits[1].length());
                    size_t length = limits[0].length();
                    for (int n = std::stoi(limits[0]); n <= std::stoi(limits[1]); ++n) {
                        slurm_nodes.push_back(main + to_string_with_leading_zeroes(n, length));
                    }
				}
			}
	   	}
    }
//    for (auto &s : slurm_nodes) {
//        std::cout << ">> " << s << std::endl;
//    }
    global::sem.wait();
    nodelist::init(1, slurm_id);
    nodelist l;
    l.load();
    for (const auto &name : slurm_nodes) {
        l.freenodes.push_back(node { name, 0 });
    }
    l.save();
    global::sem.post();
}

void show_slurm_id() {
    nodelist l;
    global::sem.wait();
    l.load();
    global::sem.post();
    std::cout << "SLURM_JOBID: " << l.slurm_id << std::endl;
}

int start(int N)
{
    nodelist l;
    int id = -1;
    while (true) {
        global::sem.wait();
        l.load();
        if (id == -1) {
            id = ++l.max_id;
            l.save();
            std::cout << "id: " << l.max_id << std::endl;
        }
        if (l.free < N) {
            global::sem.post();
            usleep(100000);
            continue;
        }
        break;
    }
    int cnt = 0;
    for (auto &n : l.freenodes) {
        if (n.id == -1)
            continue;
        n.id = id;
        l.busynodes.push_back(n);
        std::cout << "node: " << n.name << std::endl;
        if (++cnt == N)
            break;
    }
    for (auto it = l.freenodes.begin(); it != l.freenodes.end();) {
        if (it->id > 0)
            it = l.freenodes.erase(it);
        else
            ++it;
    }
    l.save();
    global::sem.post();
    return 0;
}

int stop(int N) 
{
    nodelist l;
    global::sem.wait();
    l.load();
    for (auto &n : l.busynodes) {
        if (n.id == N) {
            n.id = 0;
            l.freenodes.push_back(n);
        }
    }
    for (auto it = l.busynodes.begin(); it != l.busynodes.end();) {
        if (!it->id)
            it = l.busynodes.erase(it);
        else
            ++it;
    }
    l.save();
    global::sem.post();
    return 0;
}

void defunct(const std::string &name)
{
    nodelist l;
    global::sem.wait();
    l.load();
    for (auto &n : l.freenodes) {
        if (n.name == name) {
            n.id = -1;
        }
    }
    for (auto &n : l.busynodes) {
        if (n.name == name) {
            n.id = -1;
            l.freenodes.push_back(n);
        }
    }
    for (auto it = l.busynodes.begin(); it != l.busynodes.end();) {
        if (it->id == -1)
            it = l.busynodes.erase(it);
        else
            ++it;
    }
    l.save();
    global::sem.post();
}

void add(const std::string &name)
{
    nodelist l;
    global::sem.wait();
    l.load();
    for (auto &n : l.freenodes) {
        if (n.name == name) {
            if (n.id == -1) {
                n.id = 0;
                l.save();
                global::sem.post();
                return;
            } else {
                throw EX_ADD_DUPLICATE_NODE;
            }
        }
    }
    for (auto &n : l.busynodes) {
        if (n.name == name) {
           throw EX_ADD_DUPLICATE_NODE;
        }
    }
    l.freenodes.push_back(node { name, 0 } );
    l.save();
    global::sem.post();
}

void busyloop(const std::string &state)
{
    if (state != "on" && state != "off")
        return;
    nodelist l;
    global::sem.wait();
    l.load();
    l.busyloop = state;
    l.save();
    global::sem.post();
}
