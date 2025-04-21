#include "smokers.h"
#include <random>

SharedData* shared_data;
int semid;

void init_resources() {
  int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
  shared_data = (SharedData*)shmat(shmid, nullptr, 0);
  semid = semget(SEM_KEY, 4, 0666);
}

void cleanup() {
  shmdt(shared_data);
}

void signal_handler(int) {
  cleanup();
  exit(0);
}

int main() {
  init_resources();
  signal(SIGINT, signal_handler);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(1000000, 3000000);

  while (true) {
    sem_lock(semid, 1); // Ждем компоненты

    if (shared_data->paper && shared_data->matches) {
      shared_data->paper = false;
      shared_data->matches = false;
      std::cout << "Smoker 1 (tobacco) is smoking" << std::endl;
      sem_unlock(semid, 0); // Разрешаем агенту
      usleep(dist(gen)); // Курим
    }
  }

  return 0;
}