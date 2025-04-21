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

int agent_sem, tobacco_sem, paper_sem, matches_sem, mutex_sem;  // Изменил имя переменной

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
  tobacco_sem = semget(TOBACCO_SEM_KEY, 1, 0666);
  paper_sem = semget(PAPER_SEM_KEY, 1, 0666);
  matches_sem = semget(MATCHES_SEM_KEY, 1, 0666);
  mutex_sem = semget(MUTEX_KEY, 1, 0666);  // Используем новое имя

  if (agent_sem == -1 || tobacco_sem == -1 || paper_sem == -1 || matches_sem == -1 || mutex_sem == -1) {
    perror("semget failed");
    shmdt(shared);
    return 1;
  }

  cout << "Агент (PID: " << getpid() << ") начал работу" << endl;

  while (!stop_flag && !shared->stop_flag) {
    sem_wait(agent_sem);

    int choice = 1 + rand() % 3;

    sem_wait(mutex_sem);
    switch (choice) {
      case 1:
        shared->is_tobacco = true;
        shared->is_paper = true;
        cout << "Агент положил табак и бумагу" << endl;
        sem_post(matches_sem);
        break;
      case 2:
        shared->is_tobacco = true;
        shared->is_matches = true;
        cout << "Агент положил табак и спички" << endl;
        sem_post(paper_sem);
        break;
      case 3:
        shared->is_paper = true;
        shared->is_matches = true;
        cout << "Агент положил бумагу и спички" << endl;
        sem_post(tobacco_sem);
        break;
    }
    sem_post(mutex_sem);

    random_delay(500, 1500);
  }

  cout << "Агент (PID: " << getpid() << ") завершает работу" << endl;
  shmdt(shared);
  return 0;
}