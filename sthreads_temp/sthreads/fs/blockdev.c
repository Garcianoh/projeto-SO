// blockdev.c - Emulação de acesso à memória secundária
#include "blockdev.h"
#include <stdio.h>
#include <string.h>

void blockdev_read(const char* file, int block, void* buf) {
    printf("[BLOCKDEV] Leitura do ficheiro %s bloco %d\n", file, block);
    memset(buf, 'D', 256);
}
void blockdev_write(const char* file, int block, void* buf) {
    printf("[BLOCKDEV] Escrita no ficheiro %s bloco %d\n", file, block);
}
