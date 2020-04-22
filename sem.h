#pragma once
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

struct semaphore {
    sem_t *sem = NULL;
    bool gotit = false, inside = false;
    void unlink() {
        sem_unlink(global::semname.c_str());
    }
    bool open(bool is_init_mode = true) {
        inside = true;
        if (is_init_mode) {
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
            sem_init(sem, 1, 1);
#ifdef WITH_DEBUG            
            int value;
            sem_getvalue(sem, &value);
            std::cout << ">> semaphore: after init: " << value << std::endl;
#endif            
        } else {
            if ((sem = sem_open(global::semname.c_str(), O_EXCL)) == SEM_FAILED) {
                std::cerr << "VBBS: semaphore open error: " << global::semname.c_str() << std::endl;
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
    void _wait() {
        inside = true;
        if (sem) {
            sem_wait(sem);
            gotit = true;
            int value;
            sem_getvalue(sem, &value);
            if (value != 0) {
                throw EX_SEM_INVALID;
            }
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
    void wait() {
        int cnt = 0;
#ifdef WITH_DEBUG        
        int value;
        sem_getvalue(sem, &value);
        std::cout << ">> semaphore: before wait: " << value << std::endl;
#endif        
        while (true) {
            struct timespec ts;
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
                std::cerr << "VBBS: wait(): clock_gettime: error!" << std::endl;
                perror("clock_gettime");
                _wait();
                return;
            }
            ts.tv_sec += 10;
            inside = true;
            int s = 0;
            while ((s = sem_timedwait(sem, &ts)) == -1 && errno == EINTR) {
                continue;
            }
            inside = false;
            if (s == -1) {
                if (errno == ETIMEDOUT) {
                    if (++cnt == 100) {
                        throw EX_SEM_TIMEOUT;
                    }
                    std::cerr << "VBBS: wait(): timedwait: timeout!" << std::endl;
                    usleep(10000);

                    continue;
                } else {
                    std::cerr << "VBBS: wait(): sem_timedwait: error!" << std::endl;
                    perror("sem_timedwait");
                    return;
                }
            }
            gotit = true;
            int value;
            sem_getvalue(sem, &value);
            if (value != 0) {
                std::cerr << "VBBS: wait(): logic error: sem value is not 0 after wait() completion!" << std::endl;
                throw EX_SEM_INVALID;
            }
            break;
        }
    }

};
