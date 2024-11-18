#include "error_handling.h"
#include <cctype>
#include <cerrno>
#include <cstring>
#include <unistd.h>

int main() {
  char buffer[1024];
  ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));
  if (bytesRead < 0) {
    myPerror("Ошибка при чтении в child2");
    _exit(errno);
  }

  for (int i = 0; i < bytesRead; ++i) {
    if (isspace(buffer[i]) && buffer[i] != '\n') {
      buffer[i] = '_';
    }
  }

  if (write(STDOUT_FILENO, buffer, bytesRead) == -1) {
    myPerror("Ошибка при записи в child2");
    _exit(errno);
  }

  return 0;
}
