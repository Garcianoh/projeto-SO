// fs_ipc.c - Implementação IPC simples usando pipe para testes
#include "fs_ipc.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int pipefd[2];

void fs_ipc_init() {
    if (pipe(pipefd) < 0) {
        perror("pipe");
    }
}

// Simula recepção de um pedido (blocking read)
fs_request_t fs_ipc_receive() {
    fs_request_t req;
    read(pipefd[0], &req, sizeof(req));
    return req;
}

// Simula envio de resposta (aqui só imprime)
void fs_ipc_respond(int client, void* data, int size) {
    printf("[FS_IPC] Resposta enviada ao cliente %d (size=%d)\n", client, size);
}

// Função auxiliar para testes: enviar pedido
void fs_ipc_send(const fs_request_t* req) {
    write(pipefd[1], req, sizeof(*req));
}
