#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

std::vector<char *> split(const std::string &s) {
  std::vector<char *> v;
  std::string tmp;
  for (char c : s) {
    if (c == ' ') {
      if (!tmp.empty()) {
        v.push_back(strdup(tmp.c_str()));
        tmp.clear();
      }
    } else
      tmp += c;
  }
  if (!tmp.empty())
    v.push_back(strdup(tmp.c_str()));
  v.push_back(nullptr);
  return v;
}

int main(int argc, char **argv) {

  // Parse command line arguments: find the pipe separator "|"
  // Format: ./pipe cmd1 [args...] | cmd2 [args...]
  int i = 1;
  std::string cmd1, cmd2;

  while (argv[i] && strcmp(argv[i], "|") != 0) {
    if (!cmd1.empty())
      cmd1 += " ";
    cmd1 += argv[i];
    i++;
  }

  if (argv[i] && strcmp(argv[i], "|") == 0)
    i++;

  while (argv[i]) {
    if (!cmd2.empty())
      cmd2 += " ";
    cmd2 += argv[i];
    i++;
  }

  std::vector<char *> args1 = split(cmd1);
  std::vector<char *> args2 = split(cmd2);

  // Create a pipe for inter-process communication
  int pipefd[2];
  pipe(pipefd);

  // Fork the first child process ; child redirects out to write end of pipe, then exec
  pid_t p1 = fork();
  if (p1 == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    execvp(args1[0], args1.data());
    perror("execvp cmd1");
    exit(1);
  }

  // Fork the second child process ; child redirects in from read end of pipe, then exec
  pid_t p2 = fork();
  if (p2 == 0) {
    close(pipefd[1]);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    execvp(args2[0], args2.data());
    perror("execvp cmd2");
    exit(1);
  }

  close(pipefd[0]);
  close(pipefd[1]);

  // Wait for both children to finish
  while (wait(NULL) > 0) {
  }

  return 0;
}
