#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

pthread_mutex_t ioMutex;
sem_t threadLimit;

struct ThreadData {
  std::vector<int> *array;
  int left;
  int right;
};

void writeToStdout(const char *str) { write(STDOUT_FILENO, str, strlen(str)); }

void printArray(const std::vector<int> &array) {
  pthread_mutex_lock(&ioMutex);
  for (size_t i = 0; i < array.size(); ++i) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d ", array[i]);
    writeToStdout(buffer);
  }
  writeToStdout("\n");
  pthread_mutex_unlock(&ioMutex);
}

void mergeArrays(std::vector<int> &array, int left, int mid, int right) {
  int n1 = mid - left + 1;
  int n2 = right - mid;

  std::vector<int> leftArray(n1), rightArray(n2);
  for (int i = 0; i < n1; ++i)
    leftArray[i] = array[left + i];
  for (int i = 0; i < n2; ++i)
    rightArray[i] = array[mid + 1 + i];

  int i = 0, j = 0, k = left;
  while (i < n1 && j < n2) {
    if (leftArray[i] <= rightArray[j]) {
      array[k] = leftArray[i];
      ++i;
    } else {
      array[k] = rightArray[j];
      ++j;
    }
    ++k;
  }

  while (i < n1) {
    array[k] = leftArray[i];
    ++i;
    ++k;
  }

  while (j < n2) {
    array[k] = rightArray[j];
    ++j;
    ++k;
  }
}

void *mergeSort(void *arg) {
  ThreadData *data = (ThreadData *)arg;
  std::vector<int> &array = *(data->array);
  int left = data->left;
  int right = data->right;

  char debugBuffer[64];
  if (left >= right) {
    sem_post(&threadLimit);
    return nullptr;
  }

  int mid = left + (right - left) / 2;

  pthread_t leftThread, rightThread;
  ThreadData leftData = {&array, left, mid};
  ThreadData rightData = {&array, mid + 1, right};

  if (pthread_create(&leftThread, nullptr, mergeSort, &leftData) != 0) {
    writeToStdout(
        "Failed to create left thread, falling back to single-threaded\n");
    mergeSort(&leftData);
    sem_post(&threadLimit);
  }

  if (pthread_create(&rightThread, nullptr, mergeSort, &rightData) != 0) {
    writeToStdout(
        "Failed to create right thread, falling back to single-threaded\n");
    mergeSort(&rightData);
    sem_post(&threadLimit);
  }

  pthread_join(leftThread, nullptr);
  pthread_join(rightThread, nullptr);
  mergeArrays(array, left, mid, right);

  sem_post(&threadLimit);
  return nullptr;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    writeToStdout("Usage: ./mergeSort <maxThreads> <arraySize>\n");
    return 1;
  }

  int maxThreads = std::atoi(argv[1]);
  int arraySize = std::atoi(argv[2]);

  if (maxThreads < 1 || arraySize < 1) {
    writeToStdout("Both maxThreads and arraySize must be positive integers.\n");
    return 1;
  }

  pthread_mutex_init(&ioMutex, nullptr);
  sem_init(&threadLimit, 0, maxThreads);

  std::srand(time(nullptr));
  std::vector<int> array(arraySize);

  for (int &x : array)
    x = std::rand() % 100;

  writeToStdout("Unsorted array:\n");
  printArray(array);

  ThreadData initialData = {&array, 0, arraySize - 1};

  struct timeval start, end;
  gettimeofday(&start, nullptr);

  sem_wait(&threadLimit);
  mergeSort(&initialData);

  gettimeofday(&end, nullptr);

  double elapsedTime =
      (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

  writeToStdout("Sorted array:\n");
  printArray(array);

  char buffer[64];
  snprintf(buffer, sizeof(buffer), "Time elapsed: %.6f seconds\n", elapsedTime);
  writeToStdout(buffer);

  pthread_mutex_destroy(&ioMutex);
  sem_destroy(&threadLimit);

  return 0;
}
