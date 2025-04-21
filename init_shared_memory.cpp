#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <cstring>

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
const key_t MUTEX_SEM_KEY = 0x1239;

int main() {
  // Создаем разделяемую память
  int shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
  if (shm_id == -1) {
    perror("shmget failed");
    return 1;
  }

  SharedData* shared = (SharedData*)shmat(shm_id, nullptr, 0);
  if (shared == (void*)-1) {
    perror("shmat failed");
    return 1;
  }

  // Инициализация данных
  shared->is_tobacco = false;
  shared->is_paper = false;
  shared->is_matches = false;
  shared->stop_flag = false;

  // Создаем семафоры
  int agent_sem = semget(AGENT_SEM_KEY, 1, IPC_CREAT | 0666);
  int tobacco_sem = semget(TOBACCO_SEM_KEY, 1, IPC_CREAT | 0666);
  int paper_sem = semget(PAPER_SEM_KEY, 1, IPC_CREAT | 0666);
  int matches_sem = semget(MATCHES_SEM_KEY, 1, IPC_CREAT | 0666);
  int mutex = semget(MUTEX_SEM_KEY, 1, IPC_CREAT | 0666);

  // Инициализация семафоров
  semctl(agent_sem, 0, SETVAL, 1);  // Агент начинает первым
  semctl(tobacco_sem, 0, SETVAL, 0);
  semctl(paper_sem, 0, SETVAL, 0);
  semctl(matches_sem, 0, SETVAL, 0);
  semctl(mutex, 0, SETVAL, 1);      // Мьютекс разблокирован

  shmdt(shared);
  cout << "Ресурсы инициализированы" << endl;
  return 0;
}