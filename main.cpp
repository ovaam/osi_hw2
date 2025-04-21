#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <mutex>
#include <dispatch/dispatch.h>

using namespace std;

// Класс семафора для MacOS
class MacOSSemaphore {
 private:
  dispatch_semaphore_t sem;
 public:
  explicit MacOSSemaphore(int value) : sem(dispatch_semaphore_create(value)) {}
  ~MacOSSemaphore() { /* Деструктор - не требуется освобождение в ARC */ }

  void wait() {
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
  }

  void signal() {
    dispatch_semaphore_signal(sem);
  }
};

// Глобальные переменные
MacOSSemaphore agentSem(1);       // Посредник может начать работу сразу
MacOSSemaphore tobaccoSem(0);     // Курильщики изначально ждут
MacOSSemaphore paperSem(0);
MacOSSemaphore matchSem(0);
mutex coutMutex;                 // Мьютекс для безопасного вывода
mutex tableMutex;                // Мьютекс для доступа к таблице компонентов
bool isTobacco = false;          // Флаги компонентов на столе
bool isPaper = false;
bool isMatch = false;

// Функция для случайной задержки
void randomDelay(int min_ms, int max_ms) {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> dist(min_ms, max_ms);
  this_thread::sleep_for(chrono::milliseconds(dist(gen)));
}

// Функция посредника
void agent() {
  while (true) {
    agentSem.wait(); // Ждем, пока курильщик не закончит

    // Выбираем случайные два компонента
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, 2);
    int choice = dist(gen);

    {
      lock_guard<mutex> lock(tableMutex);
      isTobacco = false;
      isPaper = false;
      isMatch = false;

      switch (choice) {
        case 0: // Кладем табак и бумагу
          isTobacco = true;
          isPaper = true;
          {
            lock_guard<mutex> coutLock(coutMutex);
            cout << "Посредник положил табак и бумагу" << endl;
          }
          matchSem.signal(); // Будим курильщика со спичками
          break;
        case 1: // Кладем табак и спички
          isTobacco = true;
          isMatch = true;
          {
            lock_guard<mutex> coutLock(coutMutex);
            cout << "Посредник положил табак и спички" << endl;
          }
          paperSem.signal(); // Будим курильщика с бумагой
          break;
        case 2: // Кладем бумагу и спички
          isPaper = true;
          isMatch = true;
          {
            lock_guard<mutex> coutLock(coutMutex);
            cout << "Посредник положил бумагу и спички" << endl;
          }
          tobaccoSem.signal(); // Будим курильщика с табаком
          break;
      }
    }

    randomDelay(500, 2000); // Задержка перед следующим действием
  }
}

// Функции курильщиков (без параметров)
void smokerWithTobacco() {
  while (true) {
    tobaccoSem.wait();
    {
      lock_guard<mutex> lock(tableMutex);
      if (isPaper && isMatch) {
        isPaper = false;
        isMatch = false;
        {
          lock_guard<mutex> coutLock(coutMutex);
          cout << "Курильщик 1 (табак) берет бумагу и спички" << endl;
        }
        agentSem.signal();
        {
          lock_guard<mutex> coutLock(coutMutex);
          cout << "Курильщик 1 курит" << endl;
        }
        randomDelay(1000, 3000);
      }
    }
  }
}

void smokerWithPaper() {
  while (true) {
    paperSem.wait();
    {
      lock_guard<mutex> lock(tableMutex);
      if (isTobacco && isMatch) {
        isTobacco = false;
        isMatch = false;
        {
          lock_guard<mutex> coutLock(coutMutex);
          cout << "Курильщик 2 (бумага) берет табак и спички" << endl;
        }
        agentSem.signal();
        {
          lock_guard<mutex> coutLock(coutMutex);
          cout << "Курильщик 2 курит" << endl;
        }
        randomDelay(1000, 3000);
      }
    }
  }
}

void smokerWithMatch() {
  while (true) {
    matchSem.wait();
    {
      lock_guard<mutex> lock(tableMutex);
      if (isTobacco && isPaper) {
        isTobacco = false;
        isPaper = false;
        {
          lock_guard<mutex> coutLock(coutMutex);
          cout << "Курильщик 3 (спички) берет табак и бумагу" << endl;
        }
        agentSem.signal();
        {
          lock_guard<mutex> coutLock(coutMutex);
          cout << "Курильщик 3 курит" << endl;
        }
        randomDelay(1000, 3000);
      }
    }
  }
}

int main() {
  // Создание потоков
  thread agentThread(agent);
  thread smoker1(smokerWithTobacco);
  thread smoker2(smokerWithPaper);
  thread smoker3(smokerWithMatch);

  // Ожидание завершения потоков
  agentThread.join();
  smoker1.join();
  smoker2.join();
  smoker3.join();

  return 0;
}