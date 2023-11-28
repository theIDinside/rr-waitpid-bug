#include <thread>
#include <iostream>
#include <unistd.h>

int main() {
  auto pid = getpid();
  auto ppid = getppid();
  std::cout << "[child]: my pid=" << pid << ", ppid=" << ppid << std::endl;

  auto thr = std::thread{[](){
    std::cout << "hello world from thread" << std::endl;
  }};
  std::cout << "thread spawned" << std::endl;

  thr.join();
}