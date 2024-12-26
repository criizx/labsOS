#include "error_handling.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const int SHM_SIZE = 4096;
const char *SHM_NAME = "/shm_example";
const char *SEM_NAME_READ = "/sem_read";
const char *SEM_NAME_WRITE = "/sem_write";

int main() {
  char file_path[1024];

  write(STDOUT_FILENO, "Введите путь к файлу: ", 24);
  ssize_t bytes_read = read(STDIN_FILENO, file_path, sizeof(file_path) - 1);

  if (bytes_read < 0) {
    myPerror("Ошибка при чтении пути к файлу");
    _exit(errno);
  }

  if (bytes_read > 0 && file_path[bytes_read - 1] == '\n') {
    file_path[bytes_read - 1] = '\0';
  }

  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    myPerror("Ошибка при создании shared memory");
    _exit(errno);
  }

  if (ftruncate(shm_fd, SHM_SIZE) == -1) {
    myPerror("Ошибка при установке размера shared memory");
    _exit(errno);
  }

  void *shm_ptr =
      mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    myPerror("Ошибка при отображении shared memory");
    _exit(errno);
  }

  sem_t *sem_read = sem_open(SEM_NAME_READ, O_CREAT, 0666, 0);
  sem_t *sem_write = sem_open(SEM_NAME_WRITE, O_CREAT, 0666, 1);

  if (sem_read == SEM_FAILED || sem_write == SEM_FAILED) {
    myPerror("Ошибка при создании семафоров");
    _exit(errno);
  }

  pid_t pid1 = fork();
  if (pid1 == 0) {
    execlp("./child1_process", "./child1_process", nullptr);
    myPerror("Ошибка при выполнении exec для child1");
    _exit(errno);
  }

  pid_t pid2 = fork();
  if (pid2 == 0) {
    execlp("./child2_process", "./child2_process", nullptr);
    myPerror("Ошибка при выполнении exec для child2");
    _exit(errno);
  }

  int file_fd = open(file_path, O_RDONLY);
  if (file_fd == -1) {
    myPerror("Ошибка при открытии файла");
    return 1;
  }

  char input_buffer[1024];
  ssize_t file_bytes_read;

  while ((file_bytes_read = read(file_fd, input_buffer, sizeof(input_buffer))) >
         0) {
    sem_wait(sem_write);

    memcpy(shm_ptr, input_buffer, file_bytes_read);
    ((char *)shm_ptr)[file_bytes_read] = '\0';
    sem_post(sem_read);

    sem_wait(sem_write);
    write(STDOUT_FILENO, shm_ptr, strlen((char *)shm_ptr));
    sem_post(sem_read);
  }

  if (file_bytes_read < 0) {
    myPerror("Ошибка при чтении файла");
  }

  close(file_fd);

  waitpid(pid1, nullptr, 0);
  waitpid(pid2, nullptr, 0);

  munmap(shm_ptr, SHM_SIZE);
  shm_unlink(SHM_NAME);

  sem_close(sem_read);
  sem_close(sem_write);
  sem_unlink(SEM_NAME_READ);
  sem_unlink(SEM_NAME_WRITE);

  return 0;
}
