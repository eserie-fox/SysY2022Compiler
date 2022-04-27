#include <pthread.h>
#include <thread>
#include "Common.h"
#include "greet.h"

void *Grt(void *arg) {
  (void)arg;
  Greet();
  return NULL;
}

int main() {
  using namespace std;
  std::thread t(Greet);
  std::thread t2(Greet);
  pthread_t p;
  pthread_create(&p, NULL, &Grt, NULL);
  pthread_join(p, NULL);
  t.join();
  t2.join();
  return 0;
}