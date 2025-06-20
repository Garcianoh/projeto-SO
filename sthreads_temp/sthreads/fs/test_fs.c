// test_fs.c - Cliente de teste para o sistema de ficheiros
#include "fs_ipc.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    fs_ipc_init();
    fs_request_t req;
    memset(&req, 0, sizeof(req));
    req.op = 0; // FS_READ
    strcpy(req.file, "ficheiro1.txt");
    req.offset = 0;
    req.size = 32;
    req.client = 42;
    req.data = NULL; // leitura não precisa de buffer
    printf("[TEST] Enviando pedido de leitura...\n");
    fs_ipc_send(&req);
    sleep(1);

    char buffer[256];
    req.op = 1; // FS_WRITE
    strcpy(req.file, "ficheiro1.txt");
    req.offset = 0;
    req.size = 32;
    req.client = 42;
    strcpy(buffer, "dados de teste");
    req.data = buffer;
    printf("[TEST] Enviando pedido de escrita...\n");
    fs_ipc_send(&req);
    return 0;
}
