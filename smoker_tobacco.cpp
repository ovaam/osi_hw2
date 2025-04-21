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
const key_t MUTEX_KEY = 0x1239;  // Измененное имя константы

int agent_sem, tobacco_sem, mutex_sem;  // Измененное имя переменной

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

  // Подключаемся к разделяемой памяти
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

  // Получаем семафоры
  agent_sem = semget(AGENT_SEM_KEY, 1, 0666);
  tobacco_sem = semget(TOBACCO_SEM_KEY, 1, 0666);
  mutex_sem = semget(MUTEX_KEY, 1, 0666);

  if (agent_sem == -1 || tobacco_sem == -1 || mutex_sem == -1) {
    perror("semget failed");
    shmdt(shared);
    return 1;
  }

  cout << "Курильщик с табаком (PID: " << getpid() << ") начал работу" << endl;

  while (!stop_flag && !shared->stop_flag) {
    sem_wait(tobacco_sem);  // Ожидаем сигнал от агента

    sem_wait(mutex_sem);  // Захватываем мьютекс
    if (shared->is_paper && shared->is_matches) {
      // Забираем компоненты со стола
      shared->is_paper = false;
      shared->is_matches = false;
      cout << "Курильщик с табаком взял бумагу и спички" << endl;
      sem_post(mutex_sem);  // Освобождаем мьютекс

      // Процесс изготовления и курения сигареты
      cout << "Курильщик с табаком скручивает сигарету..." << endl;
      random_delay(500, 1000);
      cout << "Курильщик с табаком курит..." << endl;
      random_delay(1000, 2000);

      // Уведомляем агента, что можно класть новые компоненты
      sem_post(agent_sem);
    } else {
      sem_post(mutex_sem);  // Освобождаем мьютекс, если компоненты не те
    }
  }

  cout << "Курильщик с табаком (PID: " << getpid() << ") завершает работу" << endl;
  shmdt(shared);
  return 0;
}