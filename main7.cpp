#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dispatch/dispatch.h>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <random>
#include <cstdio>
#include <string>

using namespace std;

struct SharedData {
  dispatch_semaphore_t agent_sem;
  dispatch_semaphore_t tobacco_sem;
  dispatch_semaphore_t paper_sem;
  dispatch_semaphore_t match_sem;
  dispatch_semaphore_t io_sem;

  bool tobacco;
  bool paper;
  bool matches;
  bool running;

  char message[256];
};

SharedData* shared_data;

void signal_handler(int sig) {
  shared_data->running = false;
  printf("\nПолучен сигнал прерывания. Завершение работы...\n");
  exit(0);
}

void print_message(const string& msg) {
  dispatch_semaphore_wait(shared_data->io_sem, DISPATCH_TIME_FOREVER);
  printf("%s\n", msg.c_str());
  fflush(stdout);
  dispatch_semaphore_signal(shared_data->io_sem);
}

void agent_process() {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> dist(0, 2);

  while (shared_data->running) {
    dispatch_semaphore_wait(shared_data->agent_sem, DISPATCH_TIME_FOREVER);

    shared_data->tobacco = false;
    shared_data->paper = false;
    shared_data->matches = false;

    int choice = dist(gen);
    switch (choice) {
      case 0:
        shared_data->tobacco = true;
        shared_data->paper = true;
        print_message("Посредник положил табак и бумагу");
        dispatch_semaphore_signal(shared_data->match_sem);
        break;
      case 1:
        shared_data->tobacco = true;
        shared_data->matches = true;
        print_message("Посредник положил табак и спички");
        dispatch_semaphore_signal(shared_data->paper_sem);
        break;
      case 2:
        shared_data->paper = true;
        shared_data->matches = true;
        print_message("Посредник положил бумагу и спички");
        dispatch_semaphore_signal(shared_data->tobacco_sem);
        break;
    }

    usleep(500000 + rand() % 1500000);
  }
}

void smoker_process(int id, const string& item, dispatch_semaphore_t my_sem) {
  while (shared_data->running) {
    dispatch_semaphore_wait(my_sem, DISPATCH_TIME_FOREVER);

    bool can_smoke = false;
    if (id == 1 && shared_data->paper && shared_data->matches) {
      shared_data->paper = shared_data->matches = false;
      can_smoke = true;
    }
    else if (id == 2 && shared_data->tobacco && shared_data->matches) {
      shared_data->tobacco = shared_data->matches = false;
      can_smoke = true;
    }
    else if (id == 3 && shared_data->tobacco && shared_data->paper) {
      shared_data->tobacco = shared_data->paper = false;
      can_smoke = true;
    }

    if (can_smoke) {
      print_message("Курильщик " + to_string(id) + " (" + item + ") взял компоненты");
      print_message("Курильщик " + to_string(id) + " курит");
      dispatch_semaphore_signal(shared_data->agent_sem);
      usleep(1000000 + rand() % 2000000);
    }
  }
}

int main() {
  srand(time(nullptr));

  // Создаем разделяемую память
  shared_data = (SharedData*)mmap(NULL, sizeof(SharedData),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  // Инициализируем семафоры
  shared_data->agent_sem = dispatch_semaphore_create(1);
  shared_data->tobacco_sem = dispatch_semaphore_create(0);
  shared_data->paper_sem = dispatch_semaphore_create(0);
  shared_data->match_sem = dispatch_semaphore_create(0);
  shared_data->io_sem = dispatch_semaphore_create(1);

  shared_data->running = true;

  signal(SIGINT, signal_handler);

  // Создаем процессы курильщиков
  for (int i = 1; i <= 3; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      switch (i) {
        case 1: smoker_process(1, "табак", shared_data->tobacco_sem); break;
        case 2: smoker_process(2, "бумага", shared_data->paper_sem); break;
        case 3: smoker_process(3, "спички", shared_data->match_sem); break;
      }
      exit(0);
    }
    else if (pid < 0) {
      perror("fork failed");
      exit(1);
    }
  }

  // Родительский процесс - посредник
  agent_process();

  // Ожидаем завершения дочерних процессов
  while (wait(NULL) > 0);

  return 0;
}