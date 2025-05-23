
#include <stdio.h>
#include <stdint.h>
#include "sthread.h"
#include "sthread_user.h"

extern sthread_t sthread_create(sthread_start_func_t, void*, int);

#define NUM_FILAS 2
#define NUM_CLIENTES 10

typedef struct {
    sthread_mon_t monitor;
    int clientes_na_fila;
} fila_t;

fila_t filas[NUM_FILAS];

void* cliente(void* arg) {
    int id = (int)(intptr_t)arg;

    int f;
    int fila0, fila1;

    sthread_user_monitor_enter(filas[0].monitor);
    fila0 = filas[0].clientes_na_fila;
    sthread_user_monitor_exit(filas[0].monitor);

    sthread_user_monitor_enter(filas[1].monitor);
    fila1 = filas[1].clientes_na_fila;
    sthread_user_monitor_exit(filas[1].monitor);

    f = (fila0 <= fila1) ? 0 : 1;

    sthread_user_monitor_enter(filas[f].monitor);
    filas[f].clientes_na_fila++;
    printf("[Cliente %d] entrou na fila %d", id, f);
    fflush(stdout);

    printf("[Cliente %d] vai esperar na fila %d", id, f);
    fflush(stdout);
    sthread_user_monitor_wait(filas[f].monitor);
    printf("[Cliente %d] acordou na fila %d", id, f);
    fflush(stdout);
    sthread_user_monitor_exit(filas[f].monitor);

    printf("[Cliente %d] foi atendido na fila %d", id, f);
    fflush(stdout);
    return NULL;
}

void* empregado(void* arg) {
    int f = (int)(intptr_t)arg;

    while (1) {
        sthread_user_monitor_enter(filas[f].monitor);
        while (filas[f].clientes_na_fila == 0) {
            int outra_fila = 1 - f;

            sthread_user_monitor_enter(filas[outra_fila].monitor);
            if (filas[outra_fila].clientes_na_fila > 0) {
                sthread_user_monitor_exit(filas[outra_fila].monitor);
                sthread_user_monitor_exit(filas[f].monitor);
                f = outra_fila;
                continue;
            }
            sthread_user_monitor_exit(filas[outra_fila].monitor);

            printf("[Empregado %d] sem clientes, esperando na fila %d...", f, f);
            fflush(stdout);
            sthread_user_monitor_wait(filas[f].monitor);
            printf("[Empregado %d] acordou na fila %d...", f, f);
            fflush(stdout);
        }

        filas[f].clientes_na_fila--;
        sthread_user_monitor_signal(filas[f].monitor);
        sthread_user_monitor_exit(filas[f].monitor);

        printf("[Empregado %d] atendeu cliente na fila %d", f, f);
        fflush(stdout);
        sthread_yield();
    }

    return NULL;
}

int main() {
    printf("...........Inicializando supermercado.........\n");
    fflush(stdout);

    sthread_user_init();

    for (int i = 0; i < NUM_FILAS; i++) {
        filas[i].monitor = sthread_user_monitor_init();
        filas[i].clientes_na_fila = 0;
        sthread_create(empregado, (void*)(intptr_t)i, 10);
        printf("Empregado %d criado\n", i);
	printf("\n");
    }

    for (int i = 0; i < NUM_CLIENTES; i++) {
        sthread_create(cliente, (void*)(intptr_t)i, 5);
        printf("Cliente %d criado\n", i);
	printf("\n");
    }

    while (1) {
        sthread_yield();
    }

    return 0;
}
