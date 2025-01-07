#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

typedef void *(*allocator_create_t)(void *const memory, const size_t size);
typedef void *(*allocator_alloc_t)(void *const allocator, const size_t size);
typedef void (*allocator_free_t)(void *const allocator, void *const memory);
typedef void (*allocator_destroy_t)(void *const allocator);

void log_info(const char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void log_error(const char *msg)
{
    write(STDERR_FILENO, msg, strlen(msg));
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        log_error("Usage: <program> <path_to_allocator_library>\n");
        return 1;
    }

    void *library_handle = dlopen(argv[1], RTLD_LAZY);
    if (library_handle == NULL)
    {
        log_error("Error loading library: ");
        log_error(dlerror());
        log_error("\n");
        return 1;
    }

    allocator_create_t create_allocator = (allocator_create_t)dlsym(library_handle, "allocator_create");
    allocator_alloc_t allocate_memory = (allocator_alloc_t)dlsym(library_handle, "allocator_alloc");
    allocator_free_t free_memory = (allocator_free_t)dlsym(library_handle, "allocator_free");
    allocator_destroy_t destroy_allocator = (allocator_destroy_t)dlsym(library_handle, "allocator_destroy");

    if (!create_allocator || !allocate_memory || !free_memory || !destroy_allocator)
    {
        log_error("Error locating functions: ");
        log_error(dlerror());
        log_error("\n");
        dlclose(library_handle);
        return 1;
    }

    size_t pool_size = 8 * 1024 * 1024; 
    void *memory_pool = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_pool == MAP_FAILED)
    {
        log_error("Memory pool creation failed\n");
        dlclose(library_handle);
        return 1;
    }

    // Создание аллокатора
    void *allocator = create_allocator(memory_pool, pool_size);
    if (!allocator)
    {
        log_error("Failed to create allocator\n");
        munmap(memory_pool, pool_size);
        dlclose(library_handle);
        return 1;
    }

    // Тест 1: Выделение памяти размером 512 байт
    void *block1 = allocate_memory(allocator, 512);
    if (block1)
    {
        log_info("Test 1: Memory allocated (512 bytes)\n");
        free_memory(allocator, block1);
        log_info("Test 1: Memory freed (512 bytes)\n");
    }
    else
    {
        log_error("Test 1: Memory allocation failed\n");
    }

    // Тест 2: Попытка выделить больше памяти, чем доступно
    void *block2 = allocate_memory(allocator, 16 * 1024 * 1024); 
    if (!block2)
    {
        log_info("Test 2: Allocation failed (expected for oversized request)\n");
    }
    else
    {
        log_error("Test 2: Unexpected success in oversized allocation\n");
        free_memory(allocator, block2);
    }

    // Тест 3: Повторное выделение и освобождение памяти
    void *block3 = allocate_memory(allocator, 1024);
    if (block3)
    {
        log_info("Test 3: Memory allocated (1024 bytes)\n");
        free_memory(allocator, block3);
        log_info("Test 3: Memory freed (1024 bytes)\n");
    }
    else
    {
        log_error("Test 3: Memory allocation failed\n");
    }

    // Тест 4: Проверка выделения и освобождения памяти нескольких блоков
    void *block4 = allocate_memory(allocator, 256);
    void *block5 = allocate_memory(allocator, 128);
    if (block4 && block5)
    {
        log_info("Test 4: Two memory blocks allocated (256 and 128 bytes)\n");
        free_memory(allocator, block4);
        free_memory(allocator, block5);
        log_info("Test 4: Memory blocks freed\n");
    }
    else
    {
        log_error("Test 4: Memory allocation failed\n");
    }

    // Тест 5: Проверка на освобождение памяти после нескольких операций
    void *block6 = allocate_memory(allocator, 2048);
    free_memory(allocator, block6);
    block6 = allocate_memory(allocator, 2048); // Проверка повторного выделения
    if (block6)
    {
        log_info("Test 5: Memory reallocated (2048 bytes)\n");
        free_memory(allocator, block6);
    }
    else
    {
        log_error("Test 5: Memory allocation failed\n");
    }

    destroy_allocator(allocator);
    log_info("Allocator destroyed\n");

    munmap(memory_pool, pool_size);

    dlclose(library_handle);

    return 0;
}
