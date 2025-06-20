// fs_cow.c - Implementação copy-on-write
#include "fs_cow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* fs_cow_get_block(const char* file, int block) {
    // Simula alocação de novo bloco
    void* new_block = malloc(256);
    memset(new_block, 'C', 256);
    printf("[FS_COW] Copy-on-write para ficheiro %s bloco %d\n", file, block);
    return new_block;
}
