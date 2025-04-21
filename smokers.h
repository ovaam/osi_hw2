#ifndef SMOKERS_H
#define SMOKERS_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <string>
#include <iostream>
#include <random>

const int SHM_KEY = 1234;
const int SEM_KEY = 5678;

struct SharedData {
  bool tobacco;
  bool paper;
  bool matches;
  bool running;
};

#if defined(__linux__)
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
#endif

inline void sem_lock(int semid, int sem_num) {
  struct sembuf sb = {
      static_cast<unsigned short>(sem_num),
      -1,
      0
  };
  semop(semid, &sb, 1);
}

inline void sem_unlock(int semid, int sem_num) {
  struct sembuf sb = {
      static_cast<unsigned short>(sem_num),
      1,
      0
  };
  semop(semid, &sb, 1);
}

#endif