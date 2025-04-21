#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <random>

using namespace std;

// Структура для разделяемой памяти
struct SharedData {
  bool tobacco;
  bool paper;
  bool matches;
  bool running;
};

SharedData* shared_data;
sem_t *agent_sem, *tobacco_sem, *paper_sem, *match_sem;

// Функция для обработки сигнала прерывания
void signal_handler(int sig) {
  shared_data->running = false;
  cout << "\nПолучен сигнал прерывания. Завершение работы..." << endl;
}

// Функция посредника
void agent_process() {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> dist(0, 2);

  while (shared_data->running) {
    sem_wait(agent_sem);

    shared_data->tobacco = false;
    shared_data->paper = false;
    shared_data->matches = false;

    int choice = dist(gen);
    switch (choice) {
      case 0:
        shared_data->tobacco = true;
        shared_data->paper = true;
        cout << "Посредник положил табак и бумагу" << endl;
        sem_post(match_sem);
        break;
      case 1:
        shared_data->tobacco = true;
        shared_data->matches = true;
        cout << "Посредник положил табак и спички" << endl;
        sem_post(paper_sem);
        break;
      case 2:
        shared_data->paper = true;
        shared_data->matches = true;
        cout << "Посредник положил бумагу и спички" << endl;
        sem_post(tobacco_sem);
        break;
    }

    usleep(500000 + rand() % 1500000); // 0.5-2 секунды
  }
}

// Функция курильщика с табаком
void smoker_with_tobacco(int id) {
  while (shared_data->running) {
    sem_wait(tobacco_sem);

    if (shared_data->paper && shared_data->matches) {
      shared_data->paper = false;
      shared_data->matches = false;
      cout << "Курильщик " << id << " (табак) взял бумагу и спички" << endl;
      sem_post(agent_sem);
      cout << "Курильщик " << id << " курит" << endl;
      usleep(1000000 + rand() % 2000000); // 1-3 секунды
    }
  }
}

// Функция курильщика с бумагой
void smoker_with_paper(int id) {
  while (shared_data->running) {
    sem_wait(paper_sem);

    if (shared_data->tobacco && shared_data->matches) {
      shared_data->tobacco = false;
      shared_data->matches = false;
      cout << "Курильщик " << id << " (бумага) взял табак и спички" << endl;
      sem_post(agent_sem);
      cout << "Курильщик " << id << " курит" << endl;
      usleep(1000000 + rand() % 2000000); // 1-3 секунды
    }
  }
}

// Функция курильщика со спичками
void smoker_with_matches(int id) {
  while (shared_data->running) {
    sem_wait(match_sem);

    if (shared_data->tobacco && shared_data->paper) {
      shared_data->tobacco = false;
      shared_data->paper = false;
      cout << "Курильщик " << id << " (спички) взял табак и бумагу" << endl;
      sem_post(agent_sem);
      cout << "Курильщик " << id << " курит" << endl;
      usleep(1000000 + rand() % 2000000); // 1-3 секунды
    }
  }
}

int main() {
  // Инициализация случайного генератора
  srand(time(nullptr));

  // Регистрация обработчика сигнала
  signal(SIGINT, signal_handler);

  // Создание разделяемой памяти
  int shm_fd = shm_open("/smokers_shm", O_CREAT | O_RDWR, 0666);
  ftruncate(shm_fd, sizeof(SharedData));
  shared_data = (SharedData*)mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  shared_data->running = true;

  // Инициализация семафоров
  agent_sem = sem_open("/agent_sem", O_CREAT, 0666, 1);
  tobacco_sem = sem_open("/tobacco_sem", O_CREAT, 0666, 0);
  paper_sem = sem_open("/paper_sem", O_CREAT, 0666, 0);
  match_sem = sem_open("/match_sem", O_CREAT, 0666, 0);

  // Создание процессов
  pid_t pid = fork();
  if (pid == 0) {
    smoker_with_tobacco(1);
    exit(0);
  }

  pid = fork();
  if (pid == 0) {
    smoker_with_paper(2);
    exit(0);
  }

  pid = fork();
  if (pid == 0) {
    smoker_with_matches(3);
    exit(0);
  }

  // Родительский процесс - посредник
  agent_process();

  // Ожидание завершения дочерних процессов
  while (wait(nullptr) > 0);

  // Очистка ресурсов
  sem_close(agent_sem);
  sem_close(tobacco_sem);
  sem_close(paper_sem);
  sem_close(match_sem);
  sem_unlink("/agent_sem");
  sem_unlink("/tobacco_sem");
  sem_unlink("/paper_sem");
  sem_unlink("/match_sem");
  munmap(shared_data, sizeof(SharedData));
  shm_unlink("/smokers_shm");

  return 0;
}