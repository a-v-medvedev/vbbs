#pragma once

#include <unistd.h>


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
