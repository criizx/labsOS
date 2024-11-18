#include "error_handling.h"
#include <cctype>
#include <cerrno>
#include <cstring>
#include <unistd.h>

int main() {
  char buffer[1024];
  ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));
  if (bytesRead < 0) {
    myPerror("Ошибка при чтении в child1");
    _exit(errno);
  }

  for (int i = 0; i < bytesRead; ++i) {
    if (buffer[i] != '\n') {
      buffer[i] = tolower(buffer[i]);
    }
  }

  if (write(STDOUT_FILENO, buffer, bytesRead) == -1) {
    myPerror("Ошибка при записи в child1");
    _exit(errno);
  }

  return 0;
}
