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

// Структура сообщения
struct msg_buffer {
  long msg_type;
  int component1;
  int component2;
};

// Имена семафоров
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

int main() {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  srand(time(nullptr) ^ getpid());

  // Создаем очередь сообщений
  key_t key = ftok("agent.cpp", 'A');
  int msgid = msgget(key, 0666 | IPC_CREAT);

  // Создаем/открываем семафоры
  sem_t *agent_sem = sem_open(AGENT_SEM, O_CREAT, 0666, 1);
  sem_t *tobacco_sem = sem_open(TOBACCO_SEM, O_CREAT, 0666, 0);
  sem_t *paper_sem = sem_open(PAPER_SEM, O_CREAT, 0666, 0);
  sem_t *matches_sem = sem_open(MATCHES_SEM, O_CREAT, 0666, 0);
  sem_t *mutex = sem_open(MUTEX_SEM, O_CREAT, 0666, 1);

  cout << "Агент (PID: " << getpid() << ") начал работу" << endl;

  while (!stop_flag) {
    sem_wait(agent_sem);

    // Выбираем два случайных компонента
    int choice = 1 + rand() % 3;
    msg_buffer msg;
    msg.msg_type = 1;

    sem_wait(mutex);
    switch (choice) {
      case 1:
        msg.component1 = 1; msg.component2 = 2;
        cout << "Агент положил табак и бумагу" << endl;
        sem_post(matches_sem);
        break;
      case 2:
        msg.component1 = 1; msg.component2 = 3;
        cout << "Агент положил табак и спички" << endl;
        sem_post(paper_sem);
        break;
      case 3:
        msg.component1 = 2; msg.component2 = 3;
        cout << "Агент положил бумагу и спички" << endl;
        sem_post(tobacco_sem);
        break;
    }
    sem_post(mutex);

    // Отправляем сообщение
    msgsnd(msgid, &msg, sizeof(msg), 0);

    random_delay(500, 1500);
  }

  // Удаляем очередь сообщений
  msgctl(msgid, IPC_RMID, NULL);

  // Закрываем семафоры
  sem_close(agent_sem);
  sem_close(tobacco_sem);
  sem_close(paper_sem);
  sem_close(matches_sem);
  sem_close(mutex);

  sem_unlink(AGENT_SEM);
  sem_unlink(TOBACCO_SEM);
  sem_unlink(PAPER_SEM);
  sem_unlink(MATCHES_SEM);
  sem_unlink(MUTEX_SEM);

  cout << "Агент (PID: " << getpid() << ") завершает работу" << endl;
  return 0;
}