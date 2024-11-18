#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "error_handling.h"

int main() {
  char file_path[1024];

  write(STDOUT_FILENO, "Введите путь к файлу: ", 25);
  ssize_t bytes_read = read(STDIN_FILENO, file_path, sizeof(file_path) - 1);

  if (bytes_read < 0) {
    myPerror("Ошибка при чтении пути к файлу");
    _exit(errno);
  }

  if (bytes_read > 0 && file_path[bytes_read - 1] == '\n') {
    file_path[bytes_read - 1] = '\0';
  }

  int pipe1[2], pipe2[2], pipe3[2];

  if (pipe(pipe1) == -1) {
    myPerror("Ошибка при создании pipe1");
    _exit(errno);
  }
  if (pipe(pipe2) == -1) {
    myPerror("Ошибка при создании pipe2");
    _exit(errno);
  }
  if (pipe(pipe3) == -1) {
    myPerror("Ошибка при создании pipe3");
    _exit(errno);
  }

  pid_t pid1 = fork();
  if (pid1 < 0) {
    myPerror("Ошибка при создании первого дочернего процесса");
    _exit(errno);
  } else if (pid1 == 0) {
    dup2(pipe1[0], STDIN_FILENO);
    dup2(pipe2[1], STDOUT_FILENO);

    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe3[0]);
    close(pipe3[1]);

    execlp("./child1_process", "./child1_process", nullptr);
    myPerror("Ошибка при выполнении exec для child1");
    _exit(errno);
  }

  pid_t pid2 = fork();
  if (pid2 < 0) {
    myPerror("Ошибка при создании второго дочернего процесса");
    _exit(errno);
  } else if (pid2 == 0) {
    dup2(pipe2[0], STDIN_FILENO);
    dup2(pipe3[1], STDOUT_FILENO);

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[1]);
    close(pipe3[0]);

    execlp("./child2_process", "./child2_process", nullptr);
    myPerror("Ошибка при выполнении exec для child2");
    _exit(errno);
  }

  close(pipe1[0]);
  close(pipe2[0]);
  close(pipe2[1]);
  close(pipe3[1]);

  int file_fd = open(file_path, O_RDONLY);
  if (file_fd == -1) {
    myPerror("Ошибка при открытии файла");
    return 1;
  }

  char input_buffer[1024];
  ssize_t file_bytes_read;

  while ((file_bytes_read = read(file_fd, input_buffer, sizeof(input_buffer))) >
         0) {
    if (write(pipe1[1], input_buffer, file_bytes_read) == -1) {
      myPerror("Ошибка при записи в pipe1 в родительском процессе");
      break;
    }

    char result_buffer[1024];
    ssize_t result_read = read(pipe3[0], result_buffer, sizeof(result_buffer));
    if (result_read < 0) {
      myPerror("Ошибка при чтении из pipe3 в родительском процессе");
      break;
    } else if (result_read > 0) {
      result_buffer[result_read] = '\0';
      if (write(STDOUT_FILENO, result_buffer, result_read) == -1) {
        myPerror("Ошибка при выводе в STDOUT");
        break;
      }
    }
  }

  if (file_bytes_read < 0) {
    myPerror("Ошибка при чтении файла");
  }

  close(file_fd);
  close(pipe1[1]);
  close(pipe3[0]);

  int status;
  if (waitpid(pid1, &status, 0) == -1) {
    myPerror("Ошибка ожидания завершения child1");
  }
  if (waitpid(pid2, &status, 0) == -1) {
    myPerror("Ошибка ожидания завершения child2");
  }

  return 0;
}
