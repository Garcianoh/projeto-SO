/*
 * test-sched.c
 * Teste visual do escalonador Linux-style O(1) da biblioteca sthreads
 * Cria múltiplas threads com diferentes prioridades e nice,
 * imprime o estado das runqueues e o turno das threads.
 *
 * Para rodar: make test-sched && ./test-sched
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sthread.h>
// Protótipos explícitos para evitar warnings de declaração implícita
void sthread_dump(void);
void sthread_user_dispatcher(void);

#define NTHREADS 5

void *thread_func(void *arg) {
    int id = (int)(long)arg;
    int i;
    int nice = 0;
    for (i = 0; i < 7; ++i) {
        // Mostra prioridade e nice real da thread
        printf("[Thread %d] Execução %d (prioridade atual: %d, nice: %d)\n",
            id,
            i,
            sthread_nice(0), // retorna prioridade atual
            nice
        );
        if (i == 3 && id == 2) {
            printf("[Thread %d] Chamando sthread_nice(+5) para simular renice\n", id);
            nice = 5;
            sthread_nice(+5); // simula mudança de nice
        }
        // Chama o dump para depuração visual
        sthread_dump();
        sthread_yield();
        usleep(20000); // 20ms para facilitar leitura
    }
    printf("[Thread %d] Terminando\n", id);
    return NULL;
}

int main() {
    printf("\n--- Teste visual do escalonador Linux-style O(1) ---\n");
    sthread_init();

    // Cria threads com diferentes prioridades
    sthread_create(thread_func, (void *)(long)1, 3);   // prioridade baixa
    sthread_create(thread_func, (void *)(long)2, 7);   // prioridade média
    sthread_create(thread_func, (void *)(long)3, 12);  // prioridade alta
    sthread_create(thread_func, (void *)(long)4, 0);   // prioridade mínima
    sthread_create(thread_func, (void *)(long)5, 14);  // prioridade máxima

    // Dump inicial antes de iniciar o dispatcher
    printf("[Main] Dump inicial antes do dispatcher:\n");
    sthread_dump();

    printf("[Main] Todas as threads criadas. Iniciando dispatcher...\n");
    sthread_user_dispatcher(); // inicia o escalonador

    // Dump final após o dispatcher
    printf("[Main] Dump final após dispatcher:\n");
    sthread_dump();

    printf("[Main] Terminou dispatcher.\n");
    return 0;
}
