#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// Custom mystrdup : allocates with new[], copies string (avoid strdup and free)
char *mystrdup(const char *src);

pid_t pid = 0;

void onSIGINT(int sig) {
  const char *msg = "SIGINT recu\n";
  write(1, msg, strlen(msg));
  if (pid != 0) {
    kill(pid, SIGINT);
    int status;
    while (waitpid(-1, &status, 0) > 0) {
    }
  } else {
    return;
  }
}

int main() {
  std::string line;
  // Préparer le gestionnaire de signaux
  struct sigaction act;
  act.sa_flags = 0;
  act.sa_mask = {0};
  act.sa_handler = onSIGINT;
  sigemptyset(&act.sa_mask);
  sigaction(SIGINT, &act, nullptr);

  while (true) {
    std::cout << "mini-shell> " << std::flush;
    if (!std::getline(std::cin, line)) {
      std::cout << "\nExiting on EOF (Ctrl-D)." << std::endl;
      break;
    }

    if (line.empty())
      continue;

    // Simple parsing: split by spaces using istringstream
    std::istringstream iss(line);
    std::vector<std::string> args;
    std::string token;
    while (iss >> token) {
      args.push_back(token);
    }
    if (args.empty())
      continue;

    // Prepare C-style argv: allocate and mystrdup
    char **argv = new char *[args.size()];
    for (size_t i = 0; i < args.size(); ++i) {
      argv[i] = mystrdup(args[i].c_str());
    }

    pid = fork();
    if (pid == 0) {
      execvp(argv[0], argv);
    }
    int status;
    waitpid(pid, &status, 0);

    // cleanup argv allocations
    for (size_t i = 0; i < args.size(); ++i) {
      delete[] argv[i];
    }
    delete[] argv;
  }
  return 0;
}

char *mystrdup(const char *src) {
  if (src == nullptr)
    return nullptr;
  size_t len = strlen(src) + 1; // +1 for null terminator
  char *dest = new char[len];
  memcpy(dest, src, len); // Or strcpy if preferred
  return dest;
}
