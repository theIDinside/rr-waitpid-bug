#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <ratio>
#include <sys/ptrace.h>
#include <thread>
#include <unistd.h>

#include <condition_variable>
#include <iostream>
#include <queue>
#include <sys/wait.h>

struct event {
  bool init;
  pid_t tid;
  int wait_status;
};

static std::mutex event_queue_mutex{};
static std::mutex event_queue_wait_mutex{};
static std::condition_variable cv{};
static std::queue<event> events{};
static bool tracer_exit = false;

static std::condition_variable wait_cv;
static std::mutex wait_mutex;

static std::ios_base::fmtflags save = std::cout.flags();

#define SYNC_WRITE(...)

void push_event(event e) {
  std::lock_guard lock(event_queue_mutex);
  std::cout << "[waiter]: pushing event " << e.tid << " status=0x" << std::hex
            << e.wait_status << std::endl;
  std::cout.flags(save);
  events.push(e);
  cv.notify_all();
}

event poll_event(int timeout_ms) {
  while (events.empty()) {
    std::unique_lock lock(event_queue_wait_mutex);
    cv.wait_for(lock, std::chrono::milliseconds{timeout_ms});
    if (tracer_exit)
      std::cout << "tracer exiting" << std::endl;
  }

  event evt;
  {
    std::lock_guard lock(event_queue_mutex);
    evt = events.front();
    events.pop();
  }
  return evt;
}

int main(int argc, const char **argv) {
  std::cout << "[parent]: " << "my pid=" << getpid() << ", ppid=" << getppid()
            << std::endl;
  auto mainThread = std::jthread{[]() {
    auto pid = fork();
    char *execve_args[] = {"./tracee", nullptr};
    char *envp_args[] = {nullptr};

    switch (pid) {
    case 0:
      if (-1 == ptrace(PTRACE_TRACEME)) {
        std::perror("ptrace failed");
        _exit(-1);
      }

      if (-1 == execve("./tracee", execve_args, envp_args)) {
        perror("execve failed");
        _exit(-1);
      }
      break;
    default: {
      int stat;
      wait(NULL);
      auto opt = ptrace(PTRACE_SETOPTIONS, pid, 0,
                        PTRACE_O_TRACEEXEC | PTRACE_O_TRACECLONE |
                            PTRACE_O_TRACESYSGOOD);
      if (opt == -1) {
        perror("setoptions failed");
      }

      wait_cv.notify_all();
      ptrace(PTRACE_CONT, pid, nullptr, nullptr);

      while (!tracer_exit) {
        auto evt = poll_event(1000);
        if (WIFEXITED(evt.wait_status) && evt.tid == pid) {
          std::cout << "[tracer]: tracee exited" << std::endl;
          tracer_exit = true;
        } else {
          std::cout << "[tracer]: ptrace_cont " << evt.tid << std::endl;
          ptrace(PTRACE_CONT, evt.tid, nullptr, nullptr);
        }

      }
    } break;
    }
  }};

  std::unique_lock lock(wait_mutex);
  wait_cv.wait(lock);
  while (!tracer_exit) {
    int wait_status = 0;
    const auto res = waitpid(-1, &wait_status, __WALL);
    if (res == -1) {
      perror("waitpid failed");
      return 0;
    }
    push_event({.tid = res, .wait_status = wait_status});
  }
}