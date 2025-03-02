#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>

int main() {
  auto pid = getpid();
  auto ppid = getppid();
  std::cout << "[child]: my pid=" << pid << ", ppid=" << ppid << std::endl;

  std::vector<std::thread> threads{};
  for (auto i = 0; i < 4; ++i) {
    threads.emplace_back([i]() {
      std::cout << "hello world from thread " << gettid() << " thread number " << i << std::endl;
    });
  }
  std::cout << "thread spawned" << std::endl;
  for(auto& t : threads) {
    t.join();
  }
  std::cout << "thread joined.. exiting" << std::endl;
  return 0;
}