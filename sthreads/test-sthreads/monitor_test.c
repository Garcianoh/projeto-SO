#include <stdio.h>
#include "sthread.h"
#include "sthread_user.h"

sthread_mon_t monitor;

void* consumidor(void* arg) {
    sthread_user_monitor_enter(monitor);
    printf("[Consumidor] Esperando sinal...\n");
    fflush(stdout);

    sthread_user_monitor_wait(monitor);
    printf("[Consumidor] Recebeu sinal!\n");
    fflush(stdout);

    sthread_user_monitor_exit(monitor);
    return NULL;
}

void* produtor(void* arg) {
    sthread_user_yield(); // Deixa o consumidor entrar primeiro

    sthread_user_monitor_enter(monitor);
    printf("[Produtor] Enviando sinal...\n");
    fflush(stdout);

    sthread_user_monitor_signal(monitor);
    sthread_user_monitor_exit(monitor);
    return NULL;
}

int main() {
    printf("Iniciando main\n");
    fflush(stdout);

    sthread_user_init();
    printf("sthread_user_init OK\n");
    fflush(stdout);

    monitor = sthread_user_monitor_init();
    printf("Monitor inicializado\n");
    fflush(stdout);

    sthread_create(consumidor, NULL, 5);
    sthread_create(produtor, NULL, 10);

    for (int i = 0; i < 10; i++) {
        printf("Yield %d\n", i);
        fflush(stdout);
        sthread_yield();
    }

    return 0;
}


