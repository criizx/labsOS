#include <cerrno>
#include <cstring>
#include <unistd.h>

void myPerror(const char *msg) {
  write(STDERR_FILENO, msg, strlen(msg));
  write(STDERR_FILENO, ": ", 2);
  write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
  write(STDERR_FILENO, "\n", 1);
}
