// fs_ipc.h - Interface de IPC para o FS
#ifndef FS_IPC_H
#define FS_IPC_H
#include <stddef.h>

typedef struct {
    int op; // FS_READ, FS_WRITE, etc
    char file[128];
    int offset;
    int size;
    void* data;
    int client;
} fs_request_t;

void fs_ipc_init();
fs_request_t fs_ipc_receive();
void fs_ipc_respond(int client, void* data, int size);
void fs_ipc_send(const fs_request_t* req);

#endif
