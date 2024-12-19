#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 10

void split_command(char *cmd, char *args[]) {
  int i = 0;
  char *token = strtok(cmd, " ");

  while (token != NULL && i < MAX_ARGS - 1) {
    args[i++] = token;
    token = strtok(NULL, " ");
  }
  args[i] = NULL;
}

char *find_command_path(char *cmd) {
  // if ommand already aboslute path
  if (access(cmd, X_OK) == 0) {
    return strdup(cmd);
  }

  // check path enviroment
  char *path_env = getenv("PATH");

  if (path_env == NULL) {
    return NULL;
  }

  char *path = strdup(path_env);
  char *dir = strtok(path, ":");

  while (dir != NULL) {
    char full_path[1024];

    snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);

    if (access(full_path, X_OK) == 0) {
      free(path);
      return strdup(full_path);
    }
    dir = strtok(NULL, ":");
  }
  free(path);

  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: redir <inp> <cmd> <out>\n");
    return 1;
  }

  char *inp = argv[1];
  char *cmd = argv[2];
  char *out = argv[3];

  char *args[MAX_ARGS];
  split_command(cmd, args);

  char *cmd_path = find_command_path(args[0]);

  if (cmd_path == NULL) {
    fprintf(stderr, "Error: Command not found: %s\n", args[0]);
    return 1;
  }

  pid_t pid = fork();

  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) { // child process
    // redirect input if needed
    if (strcmp(inp, "-") != 0) {
      int inp_fd = open(inp, O_RDONLY);
      if (inp_fd < 0) {
        perror("open input file");
        exit(1);
      }
      dup2(inp_fd, STDIN_FILENO);
      close(inp_fd);
    }

    // redirect output if needed
    if (strcmp(out, "-") != 0) {
      int out_fd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);

      if (out_fd < 0) {
        perror("open output file");
        exit(1);
      }
      dup2(out_fd, STDOUT_FILENO);
      close(out_fd);
    }

    // execute
    execv(cmd_path, args);
    perror("execv");
    exit(1);

  } else {
    wait(NULL);
  }

  free(cmd_path);
  return 0;
}