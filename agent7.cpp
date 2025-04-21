#include "smokers.h"
#include <random>

SharedData* shared_data;
int semid;

void init_resources() {
  // Создаем разделяемую память
  int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
  shared_data = (SharedData*)shmat(shmid, nullptr, 0);

  // Создаем семафоры (4 семафора)
  semid = semget(SEM_KEY, 4, IPC_CREAT | 0666);

  // Инициализируем семафоры
#if defined(__linux__)
  union semun arg;
    arg.val = 1;
    semctl(semid, 0, SETVAL, arg); // Семафор агента
    arg.val = 0;
    semctl(semid, 1, SETVAL, arg); // Табак
    semctl(semid, 2, SETVAL, arg); // Бумага
    semctl(semid, 3, SETVAL, arg); // Спички
#endif

  shared_data->running = true;
}

void cleanup() {
  shmdt(shared_data);
}

void signal_handler(int) {
  shared_data->running = false;
  std::cout << "Agent stopping..." << std::endl;
}

int main() {
  init_resources();
  signal(SIGINT, signal_handler);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, 2);

  while (shared_data->running) {
    sem_lock(semid, 0); // Ждем разрешения

    shared_data->tobacco = false;
    shared_data->paper = false;
    shared_data->matches = false;

    int choice = dist(gen);
    switch(choice) {
      case 0:
        shared_data->tobacco = true;
        shared_data->paper = true;
        std::cout << "Agent placed tobacco and paper" << std::endl;
        sem_unlock(semid, 3); // Разбудить курильщика со спичками
        break;
      case 1:
        shared_data->tobacco = true;
        shared_data->matches = true;
        std::cout << "Agent placed tobacco and matches" << std::endl;
        sem_unlock(semid, 2); // Разбудить курильщика с бумагой
        break;
      case 2:
        shared_data->paper = true;
        shared_data->matches = true;
        std::cout << "Agent placed paper and matches" << std::endl;
        sem_unlock(semid, 1); // Разбудить курильщика с табаком
        break;
    }

    usleep(500000 + rand() % 1500000);
  }

  cleanup();
  return 0;
}