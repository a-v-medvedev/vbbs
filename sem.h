#pragma once
#include <fcntl.h>
#include <semaphore.h>

struct semaphore {
    sem_t *sem = NULL;
    bool gotit = false, inside = false;
    bool open() {
        inside = true;
        if ((sem = sem_open(global::semname.c_str(), O_CREAT | O_EXCL, 0600, 1)) == SEM_FAILED) {
            if (errno == EEXIST) {
                if ((sem = sem_open(global::semname.c_str(), O_EXCL)) == SEM_FAILED) {
                    std::cerr << "VBBS: existing semaphore open error: " << global::semname.c_str() << std::endl;
                    perror("VBBS");
                    inside = false;
                    return false;
                }
            } else {
                std::cerr << "VBBS: semaphore create error: " << global::semname.c_str() << std::endl;
                perror("VBBS");
                inside = false;
                return false;
            }
        }
        inside = false;
        return true;
    }
    void close() {
        inside = true;
        if (sem && gotit) {
            sem_post(sem);
        }
        sem_close(sem);
        sem = NULL;
        inside = false;
    }
    void wait() {
        inside = true;
        if (sem) {
            sem_wait(sem);
            gotit = true;
        }
        inside = false;
    }
    void post() {
        inside = true;
        if (sem && gotit) {
            sem_post(sem);
            gotit = false;
        }
        inside = false;
    }
};
