#include <iostream>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <cstring>
#include <ctime>
#include <cstdlib>

using namespace std;

struct SharedData {
  bool is_tobacco;
  bool is_paper;
  bool is_matches;
  bool stop_flag;
};

const key_t SHM_KEY = 0x1234;
const key_t AGENT_SEM_KEY = 0x1235;
const key_t TOBACCO_SEM_KEY = 0x1236;
const key_t PAPER_SEM_KEY = 0x1237;
const key_t MATCHES_SEM_KEY = 0x1238;
const key_t MUTEX_KEY = 0x1239;  // Изменил имя константы

int agent_sem, paper_sem, mutex_sem;  // Изменил имя переменной

volatile sig_atomic_t stop_flag = 0;
void signal_handler(int signum) {
  stop_flag = 1;
}

void sem_wait(int semid) {
  struct sembuf op = {0, -1, 0};
  semop(semid, &op, 1);
}

void sem_post(int semid) {
  struct sembuf op = {0, 1, 0};
  semop(semid, &op, 1);
}

void random_delay(int min_ms, int max_ms) {
  int delay_ms = min_ms + rand() % (max_ms - min_ms + 1);
  usleep(delay_ms * 1000);
}

int main() {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  srand(time(nullptr) ^ getpid());

  int shm_id = shmget(SHM_KEY, sizeof(SharedData), 0666);
  if (shm_id == -1) {
    perror("shmget failed");
    return 1;
  }

  SharedData* shared = (SharedData*)shmat(shm_id, nullptr, 0);
  if (shared == (void*)-1) {
    perror("shmat failed");
    return 1;
  }

  agent_sem = semget(AGENT_SEM_KEY, 1, 0666);
  paper_sem = semget(PAPER_SEM_KEY, 1, 0666);
  mutex_sem = semget(MUTEX_KEY, 1, 0666);

  if (agent_sem == -1 || paper_sem == -1 || mutex_sem == -1) {
    perror("semget failed");
    shmdt(shared);
    return 1;
  }

  cout << "Курильщик с бумагой (PID: " << getpid() << ") начал работу" << endl;

  while (!stop_flag && !shared->stop_flag) {
    sem_wait(paper_sem);

    sem_wait(mutex_sem);
    if (shared->is_tobacco && shared->is_matches) {
      shared->is_tobacco = false;
      shared->is_matches = false;
      cout << "Курильщик с бумагой взял табак и спички" << endl;
      sem_post(mutex_sem);

      cout << "Курильщик с бумагой скручивает сигарету..." << endl;
      random_delay(500, 1000);
      cout << "Курильщик с бумагой курит..." << endl;
      random_delay(1000, 2000);

      sem_post(agent_sem);
    } else {
      sem_post(mutex_sem);
    }
  }

  cout << "Курильщик с бумагой (PID: " << getpid() << ") завершает работу" << endl;
  shmdt(shared);
  return 0;
}