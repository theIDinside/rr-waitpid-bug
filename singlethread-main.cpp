#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

struct event {
  pid_t tid;
  int wait_status;
};

static bool tracer_exit = false;

static constexpr auto tracee_event(int wait_status) {
  int stopsig = WSTOPSIG(wait_status);

  // Check if the status indicates a ptrace event
  if (stopsig == SIGTRAP && (wait_status >> 16) != 0) {
    switch (wait_status >> 16) {
    case PTRACE_EVENT_FORK:
      return "PTRACE_EVENT_FORK";
    case PTRACE_EVENT_VFORK:
      return "PTRACE_EVENT_VFORK";
    case PTRACE_EVENT_CLONE:
      return "PTRACE_EVENT_CLONE";
    case PTRACE_EVENT_EXEC:
      return "PTRACE_EVENT_EXEC";
    case PTRACE_EVENT_VFORK_DONE:
      return "PTRACE_EVENT_VFORK_DONE";
    case PTRACE_EVENT_EXIT:
      return "PTRACE_EVENT_EXIT";
    case PTRACE_EVENT_SECCOMP:
      return "PTRACE_EVENT_SECCOMP";
    case PTRACE_EVENT_STOP:
      return "PTRACE_EVENT_STOP";
    default:
      break;
    }
  }
  return "DONT_CARE";
}

int main(int argc, const char **argv) {
  std::cout << "[parent]: " << "my pid=" << getpid() << ", ppid=" << getppid()
            << std::endl;

  auto pid = fork();
  const char *execve_args[] = {"./tracee", nullptr};
  const char *envp_args[] = {nullptr};

  switch (pid) {
  case 0:
    if (-1 == ptrace(PTRACE_TRACEME)) {
      std::perror("ptrace failed");
      _exit(-1);
    }

    if (-1 == execve(const_cast<char*>(execve_args[0]), const_cast<char**>(execve_args), const_cast<char**>(envp_args))) {
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

    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    static std::ios_base::fmtflags save = std::cout.flags();
    while (!tracer_exit) {
      event evt{};
      evt.tid = waitpid(-1, &evt.wait_status, __WALL);
      if (evt.tid == -1) {
        perror("waitpid failed");
        _exit(-1);
      }

      std::cout << "[waiter]: wait status event " << evt.tid << " status=0x"
                << std::hex << evt.wait_status
                << " event: " << tracee_event(evt.wait_status) << std::endl;
      std::cout.flags(save);

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
}