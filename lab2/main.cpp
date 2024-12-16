#include <iostream>
#include <vector>
#include <pthread.h>
#include <mutex>
#include <algorithm>
#include <sys/time.h>
#include <cstdlib>
#include <cmath>

std::mutex io_mutex; // Для синхронизации вывода

struct ThreadData {
    std::vector<int> *array;
    int left;
    int right;
    int max_threads;
};

void merge(std::vector<int> &array, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    std::vector<int> L(n1), R(n2);
    for (int i = 0; i < n1; ++i)
        L[i] = array[left + i];
    for (int i = 0; i < n2; ++i)
        R[i] = array[mid + 1 + i];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            array[k] = L[i];
            ++i;
        } else {
            array[k] = R[j];
            ++j;
        }
        ++k;
    }

    while (i < n1) {
        array[k] = L[i];
        ++i;
        ++k;
    }

    while (j < n2) {
        array[k] = R[j];
        ++j;
        ++k;
    }
}

void *merge_sort(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    std::vector<int> &array = *(data->array);
    int left = data->left;
    int right = data->right;
    int max_threads = data->max_threads;

    if (left >= right)
        pthread_exit(nullptr);

    int mid = left + (right - left) / 2;

    pthread_t left_thread, right_thread;
    ThreadData left_data = {&array, left, mid, max_threads / 2};
    ThreadData right_data = {&array, mid + 1, right, max_threads / 2};

    bool use_threads = max_threads > 1;

    if (use_threads) {
        pthread_create(&left_thread, nullptr, merge_sort, &left_data);
        pthread_create(&right_thread, nullptr, merge_sort, &right_data);
        pthread_join(left_thread, nullptr);
        pthread_join(right_thread, nullptr);
    } else {
        merge_sort(&left_data);
        merge_sort(&right_data);
    }

    merge(array, left, mid, right);
    pthread_exit(nullptr);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <max_threads> <array_size>\n";
        return 1;
    }

    int max_threads = std::atoi(argv[1]);
    int array_size = std::atoi(argv[2]);

    if (max_threads < 1 || array_size < 1) {
        std::cerr << "Both max_threads and array_size must be positive integers.\n";
        return 1;
    }

    std::srand(std::time(nullptr));
    std::vector<int> array(array_size);

    for (int &x : array)
        x = std::rand() % 100;

    {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "Unsorted array:\n";
        for (int x : array)
            std::cout << x << " ";
        std::cout << "\n";
    }

    ThreadData initial_data = {&array, 0, array_size - 1, max_threads};

    struct timeval start, end;
    gettimeofday(&start, nullptr);
    merge_sort(&initial_data);
    gettimeofday(&end, nullptr);

    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "Sorted array:\n";
        for (int x : array)
            std::cout << x << " ";
        std::cout << "\n";
        std::cout << "Time elapsed: " << elapsed_time << " seconds\n";
    }

    return 0;
}
