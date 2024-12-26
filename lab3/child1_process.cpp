#include "error_handling.h"
#include <cctype>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>

const int SHM_SIZE = 4096;
const char *SHM_NAME = "/shm_example";
const char *SEM_NAME_READ = "/sem_read";
const char *SEM_NAME_WRITE = "/sem_write";

int main() {
  int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
  if (shm_fd == -1) {
    myPerror("Ошибка при открытии shared memory в child1");
    _exit(errno);
  }

  void *shm_ptr =
      mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    myPerror("Ошибка при отображении shared memory в child1");
    _exit(errno);
  }

  sem_t *sem_read = sem_open(SEM_NAME_READ, 0);
  sem_t *sem_write = sem_open(SEM_NAME_WRITE, 0);

  while (true) {
    sem_wait(sem_read);
    char *data = (char *)shm_ptr;

    for (int i = 0; data[i] != '\0'; ++i) {
      data[i] = tolower(data[i]);
    }

    sem_post(sem_write);
  }

  return 0;
}
