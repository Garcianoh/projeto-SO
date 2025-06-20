// fs_core.h - Núcleo do sistema de ficheiros
#ifndef FS_CORE_H
#define FS_CORE_H
#include "fs_ipc.h"

void fs_core_init();
void fs_core_handle_request(void* arg);

#endif
