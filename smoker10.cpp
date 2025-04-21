#include <iostream>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <signal.h>
#include <cstring>
#include <ctime>
#include <cstdlib>

using namespace std;

struct msg_buffer {
  long msg_type;
  int component1;
  int component2;
};

const char* AGENT_SEM = "/agent_sem";
const char* TOBACCO_SEM = "/tobacco_sem";
const char* PAPER_SEM = "/paper_sem";
const char* MATCHES_SEM = "/matches_sem";
const char* MUTEX_SEM = "/mutex_sem";

volatile sig_atomic_t stop_flag = 0;
void signal_handler(int signum) {
  stop_flag = 1;
}

void random_delay(int min_ms, int max_ms) {
  usleep((min_ms + rand() % (max_ms - min_ms + 1)) * 1000);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <smoker_id (1-3)>" << endl;
    return 1;
  }

  int smoker_id = atoi(argv[1]);
  if (smoker_id < 1 || smoker_id > 3) {
    cerr << "Invalid smoker ID. Must be 1, 2 or 3" << endl;
    return 1;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  srand(time(nullptr) ^ getpid() ^ smoker_id);

  // Подключаемся к очереди сообщений
  key_t key = ftok("agent.cpp", 'A');
  int msgid = msgget(key, 0666);

  // Открываем семафоры
  sem_t *agent_sem = sem_open(AGENT_SEM, 0);
  sem_t *tobacco_sem = sem_open(TOBACCO_SEM, 0);
  sem_t *paper_sem = sem_open(PAPER_SEM, 0);
  sem_t *matches_sem = sem_open(MATCHES_SEM, 0);
  sem_t *mutex = sem_open(MUTEX_SEM, 0);

  cout << "Курильщик " << smoker_id << " (PID: " << getpid() << ") начал работу" << endl;

  while (!stop_flag) {
    sem_t *my_sem;
    switch (smoker_id) {
      case 1: my_sem = tobacco_sem; break;
      case 2: my_sem = paper_sem; break;
      case 3: my_sem = matches_sem; break;
    }

    sem_wait(my_sem);

    // Получаем сообщение
    msg_buffer msg;
    msgrcv(msgid, &msg, sizeof(msg), 1, 0);

    cout << "Курильщик " << smoker_id << " получил компоненты: "
         << msg.component1 << " и " << msg.component2 << endl;

    bool can_smoke = false;
    switch (smoker_id) {
      case 1: can_smoke = (msg.component1 == 2 && msg.component2 == 3) ||
            (msg.component1 == 3 && msg.component2 == 2); break;
      case 2: can_smoke = (msg.component1 == 1 && msg.component2 == 3) ||
            (msg.component1 == 3 && msg.component2 == 1); break;
      case 3: can_smoke = (msg.component1 == 1 && msg.component2 == 2) ||
            (msg.component1 == 2 && msg.component2 == 1); break;
    }

    if (can_smoke) {
      cout << "Курильщик " << smoker_id << " скручивает сигарету..." << endl;
      random_delay(500, 1000);
      cout << "Курильщик " << smoker_id << " курит..." << endl;
      random_delay(1000, 2000);
      sem_post(agent_sem);
    }
  }

  sem_close(agent_sem);
  sem_close(tobacco_sem);
  sem_close(paper_sem);
  sem_close(matches_sem);
  sem_close(mutex);

  cout << "Курильщик " << smoker_id << " (PID: " << getpid() << ") завершает работу" << endl;
  return 0;
}