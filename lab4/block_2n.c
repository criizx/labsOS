#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


#define MIN_BLOCK_SIZE 16 // Минимальный размер блока
#define MAX_BLOCK_SIZE(size) ((size < 32) ? 32 : size)

int compute_log2(int value)
{
    if (value == 0)
    {
        return -1;
    }
    int res = 0;
    while (value > 1)
    {
        value >>= 1; // на 2 /
        res++;
    }
    return res;
}

// Структура для заголовка блока памяти
typedef struct BlockMetadata
{
    struct BlockMetadata *next_block;
} BlockMetadata;

// Структура аллокатора
typedef struct Allocator
{
    BlockMetadata **free_lists;
    size_t num_lists;
    void *base_addr;
    size_t total_size;
} Allocator;

Allocator *allocator_create(void *memory, size_t size)
{
    if (memory == NULL || size < sizeof(Allocator))
    {
        return NULL;
    }
    Allocator *allocator = (Allocator *)memory;
    allocator->base_addr = memory;
    allocator->total_size = size;

    size_t max_block_size = MAX_BLOCK_SIZE(size);
    size_t min_block_size = sizeof(BlockMetadata) + MIN_BLOCK_SIZE;

    allocator->num_lists = (size_t)floor(compute_log2(max_block_size) / 2) + 3;
    allocator->free_lists =
        (BlockMetadata **)((char *)memory + sizeof(Allocator));

    for (size_t i = 0; i < allocator->num_lists; i++)
    {
        allocator->free_lists[i] = NULL;
    }

    void *current_block = (char *)memory + sizeof(Allocator) +
                          allocator->num_lists * sizeof(BlockMetadata *);
    size_t remaining_memory =
        size - sizeof(Allocator) - allocator->num_lists * sizeof(BlockMetadata *);

    size_t block_size = MIN_BLOCK_SIZE;
    while (remaining_memory >= min_block_size)
    {
        if (block_size > remaining_memory)
        {
            break;
        }

        if (block_size > max_block_size)
        {
            break;
        }

        if (remaining_memory >= (block_size + sizeof(BlockMetadata)) * 2)
        {
            for (int i = 0; i < 2; i++)
            {
                BlockMetadata *header = (BlockMetadata *)current_block;
                size_t index;
                if (size == 0)
                {
                    index = 0;
                }
                else
                {
                    index = (size_t)compute_log2(block_size);
                }

                header->next_block = allocator->free_lists[index];
                allocator->free_lists[index] = header;

                current_block = (char *)current_block + block_size;
                remaining_memory -= block_size;
            }
        }
        else
        {
            BlockMetadata *header = (BlockMetadata *)current_block;
            size_t index;
            if (size == 0)
            {
                index = 0;
            }
            else
            {
                index = (size_t)compute_log2(block_size);
            }

            header->next_block = allocator->free_lists[index];
            allocator->free_lists[index] = header;

            current_block = (char *)current_block + remaining_memory;
            remaining_memory = 0;
        }

        block_size <<= 1;
    }
    return allocator;
}

// Функция выделения памяти
void *allocator_alloc(Allocator *allocator, size_t size)
{
    if (allocator==NULL || size == 0)
    {
        return NULL;
    }

    size_t index = (size == 0) ? 0 : compute_log2(size) + 1;
    if (index >= allocator->num_lists)
    {
        index = allocator->num_lists;
    }
    bool found_block = false;
    if (allocator->free_lists[index] == NULL)
    {
        while (index <= allocator->num_lists)
        {
            if (allocator->free_lists[index] != NULL)
            {
                found_block = true;
                break;
            }
            else
            {
                ++index;
            }
        }
        if (!found_block)
        {
            return NULL;
        }
    }

    BlockMetadata *block = allocator->free_lists[index];
    allocator->free_lists[index] = block->next_block;

    return (void *)((char *)block + sizeof(BlockMetadata));
}

// Функция освобождения памяти
void allocator_free(Allocator *allocator, void *ptr)
{
    if (!allocator || !ptr)
    {
        return;
    }

    BlockMetadata *block = (BlockMetadata *)((char *)ptr - sizeof(BlockMetadata));
    size_t block_size_in_bytes =
        (char *)block + sizeof(BlockMetadata) - (char *)allocator->base_addr;
    size_t current_block_size = 32;

    while (current_block_size <= block_size_in_bytes)
    {
        size_t next_size = current_block_size << 1;
        if (next_size > block_size_in_bytes)
        {
            break;
        }
        current_block_size = next_size;
    }

    size_t index = (block_size_in_bytes == 0) ? 0 : (size_t)compute_log2(current_block_size);
    if (index >= allocator->num_lists)
    {
        index = allocator->num_lists - 1;
    }

    block->next_block = allocator->free_lists[index];
    allocator->free_lists[index] = block;
}

// Функция уничтожения аллокатора
void allocator_destroy(Allocator *allocator)
{
    if (allocator)
    {
        munmap(allocator->base_addr, allocator->total_size);
    }
}