#include <stddef.h>

typedef struct Block {
    struct Block *next;  
} Block;

typedef struct MemoryAllocator {
    void *memory_start;
    size_t memory_size;
    Block *available_blocks_head;  
} MemoryAllocator;

size_t calculate_aligned_size(size_t size, size_t alignment) {  
    if (alignment == 0) {
        return size;
    }
    return (size + (alignment - 1)) & ~(alignment - 1);  
}

MemoryAllocator *allocator_create(void *memory_pool, size_t total_size) {
    if (memory_pool == NULL || total_size < sizeof(MemoryAllocator)) {
        return NULL;
    }

    MemoryAllocator *allocator = (MemoryAllocator *)memory_pool;
    allocator->memory_start = (char *)memory_pool + sizeof(MemoryAllocator);
    allocator->memory_size = total_size - sizeof(MemoryAllocator);
    allocator->available_blocks_head = (Block *)allocator->memory_start;

    if (allocator->available_blocks_head != NULL) {
        allocator->available_blocks_head->next = NULL;
    }

    return allocator;
}

void allocator_destroy(MemoryAllocator *allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->memory_start = NULL;
    allocator->memory_size = 0;
    allocator->available_blocks_head = NULL;
}

void *allocator_alloc(MemoryAllocator *allocator, size_t size) {
    if (allocator == NULL || size == 0) {
        return NULL;
    }

    size_t aligned_size = calculate_aligned_size(size, 8); // Выравнивание на 8 байт
    Block *previous_block = NULL;
    Block *current_block = allocator->available_blocks_head;

    while (current_block != NULL) {
        if (aligned_size <= allocator->memory_size) {
            if (previous_block != NULL) {
                previous_block->next = current_block->next;
            } else {
                allocator->available_blocks_head = current_block->next;
            }
            return current_block;
        }

        previous_block = current_block;
        current_block = current_block->next;
    }

    return NULL;
}

// Функция для освобождения памяти
void allocator_free(MemoryAllocator *allocator, void *memory_block) {
    if (allocator == NULL || memory_block == NULL) {
        return;
    }

    Block *block_to_free = (Block *)memory_block;
    block_to_free->next = allocator->available_blocks_head;
    allocator->available_blocks_head = block_to_free;
}
