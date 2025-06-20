// fs_sync.c - Sincronização eficiente
#include "fs_sync.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>

// Um lock global para simplificar
static pthread_mutex_t fs_mutex = PTHREAD_MUTEX_INITIALIZER;

void fs_sync_lock(const char* file) {
    printf("[FS_SYNC] Lock no ficheiro %s\n", file);
    pthread_mutex_lock(&fs_mutex);
}
void fs_sync_unlock(const char* file) {
    printf("[FS_SYNC] Unlock no ficheiro %s\n", file);
    pthread_mutex_unlock(&fs_mutex);
}
