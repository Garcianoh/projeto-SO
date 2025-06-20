// fs_daemon.c - Processo principal do sistema de ficheiros
// Recebe pedidos via IPC, cria uma sthread para cada pedido
#include "fs_ipc.h"
#include "fs_core.h"
#include <stdio.h>

int main() {
    printf("[FS_DAEMON] Servidor de sistema de ficheiros iniciado.\n");
    fs_ipc_init();
    fs_core_init();
    while (1) {
        fs_request_t req = fs_ipc_receive();
        // Para teste sequencial, chama diretamente
        fs_core_handle_request((void*)&req);
    }
    return 0;
}
