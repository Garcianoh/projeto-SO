// fs_cache.c - Implementação da cache de blocos
#include "fs_cache.h"
#include <stdio.h>
#include <string.h>

#define CACHE_SIZE 16

typedef struct {
    char file[128];
    int offset;
    int size;
    char data[256];
    int valid;
} cache_entry_t;

static cache_entry_t cache[CACHE_SIZE];

void* fs_cache_read(const char* file, int offset, int size) {
    for (int i = 0; i < CACHE_SIZE; ++i) {
        if (cache[i].valid && strcmp(cache[i].file, file) == 0 && cache[i].offset == offset && cache[i].size == size) {
            printf("[FS_CACHE] HIT: %s offset %d\n", file, offset);
            return cache[i].data;
        }
    }
    printf("[FS_CACHE] MISS: %s offset %d\n", file, offset);
    int idx = offset % CACHE_SIZE;
    strncpy(cache[idx].file, file, 128);
    cache[idx].offset = offset;
    cache[idx].size = size;
    memset(cache[idx].data, 'A', size);
    cache[idx].valid = 1;
    return cache[idx].data;
}

void fs_cache_write(const char* file, int offset, int size, void* data) {
    int idx = offset % CACHE_SIZE;
    strncpy(cache[idx].file, file, 128);
    cache[idx].offset = offset;
    cache[idx].size = size;
    memcpy(cache[idx].data, data, size);
    cache[idx].valid = 1;
    printf("[FS_CACHE] WRITE: %s offset %d\n", file, offset);
}
