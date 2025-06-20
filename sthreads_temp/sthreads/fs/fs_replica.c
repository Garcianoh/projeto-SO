// fs_replica.c - Implementação da replicação
#include "fs_replica.h"
#include <stdio.h>

void fs_replica_write(const char* file, int block, void* data) {
    // Simula escrita em dois discos
    printf("[FS_REPLICA] Escreve %s bloco %d em disco 1\n", file, block);
    printf("[FS_REPLICA] Escreve %s bloco %d em disco 2\n", file, block);
}
