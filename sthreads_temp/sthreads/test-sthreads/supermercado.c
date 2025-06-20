/*
 * supermercado.c - Implementação do Problema do Supermercado usando sthreads e monitores
 *
 * Cada caixa é protegida por um monitor. Clientes escolhem a fila mais curta.
 * Funcionários atendem clientes da sua fila ou de outra, ou bloqueiam-se se não houver clientes.
 *
 * Para compilar: adicione este ficheiro ao seu Makefile e use as flags da biblioteca sthreads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sthread.h>
#include "../sthread_lib/sthread_user.h"
void sthread_dump(void);

#define N_CAIXAS 3
#define N_CLIENTES 12

// Estrutura que representa cada caixa do supermercado
// Protegida por um monitor para garantir exclusão mútua
// clientes_na_fila: número de clientes à espera
// clientes_atendidos: estatística de quantos foram atendidos
// monitor: garante acesso exclusivo à fila
typedef struct {
    sthread_mon_t monitor;
    int clientes_na_fila;
    int clientes_atendidos;
} caixa_t;

caixa_t caixas[N_CAIXAS];

// Função auxiliar: retorna a fila com menos clientes
int escolher_fila_menos_clientes() {
    int min = caixas[0].clientes_na_fila;
    int idx = 0;
    for (int i = 1; i < N_CAIXAS; ++i) {
        if (caixas[i].clientes_na_fila < min) {
            min = caixas[i].clientes_na_fila;
            idx = i;
        }
    }
    return idx;
}

// Função auxiliar: retorna uma fila (diferente da do funcionário) que tenha clientes, ou -1
int procurar_fila_com_clientes(int exceto) {
    for (int i = 0; i < N_CAIXAS; ++i) {
        if (i != exceto && caixas[i].clientes_na_fila > 0)
            return i;
    }
    return -1;
}

// Tarefa do cliente
void* cliente(void* arg) {
    int id = (int)(long)arg;
    int caixa_escolhida = escolher_fila_menos_clientes();

    // Cliente entra na fila escolhida
    sthread_user_monitor_enter(caixas[caixa_escolhida].monitor);
    caixas[caixa_escolhida].clientes_na_fila++;
    printf("[Cliente %d] Entrou na fila %d (clientes na fila agora: %d)\n", id, caixa_escolhida, caixas[caixa_escolhida].clientes_na_fila);
    // Espera ser atendido por um funcionário
    sthread_user_monitor_wait(caixas[caixa_escolhida].monitor);
    caixas[caixa_escolhida].clientes_na_fila--;
    printf("[Cliente %d] Foi atendido e saiu da fila %d\n", id, caixa_escolhida);
    sthread_user_monitor_exit(caixas[caixa_escolhida].monitor);
    return NULL;
}

// Tarefa do funcionário
void* funcionario(void* arg) {
    int id = (int)(long)arg;
    while (1) {
        int fila = id; // Atende sua própria fila prioritariamente
        int atendeu = 0;

        // 1. Tenta atender cliente na sua própria fila
        sthread_user_monitor_enter(caixas[fila].monitor);
        if (caixas[fila].clientes_na_fila > 0) {
            sthread_user_monitor_signal(caixas[fila].monitor);
            caixas[fila].clientes_atendidos++;
            printf("[Funcionario %d] Atendeu cliente na sua fila %d (restam %d clientes)\n", id, fila, caixas[fila].clientes_na_fila-1);
            atendeu = 1;
            sthread_user_monitor_exit(caixas[fila].monitor);
            sthread_user_sleep(1); // Simula tempo de atendimento
        } else {
            sthread_user_monitor_exit(caixas[fila].monitor);
            // 2. Se sua fila está vazia, procura outra fila com clientes
            int outra = procurar_fila_com_clientes(id);
            if (outra != -1) {
                sthread_user_monitor_enter(caixas[outra].monitor);
                if (caixas[outra].clientes_na_fila > 0) {
                    sthread_user_monitor_signal(caixas[outra].monitor);
                    caixas[outra].clientes_atendidos++;
                    printf("[Funcionario %d] Fila %d vazia, foi atender cliente na fila %d (restam %d clientes)\n", id, fila, outra, caixas[outra].clientes_na_fila-1);
                    atendeu = 1;
                }
                sthread_user_monitor_exit(caixas[outra].monitor);
                sthread_user_sleep(1);
            }
        }
        // 3. Se não atendeu ninguém, bloqueia-se à espera de clientes na sua fila
        if (!atendeu) {
            sthread_user_monitor_enter(caixas[fila].monitor);
            printf("[Funcionario %d] Todas as filas vazias, bloqueado à espera de clientes na fila %d\n", id, fila);
            sthread_user_monitor_wait(caixas[fila].monitor);
            sthread_user_monitor_exit(caixas[fila].monitor);
        }
    }
    return NULL;
}

int main() {
    printf("\n=== Supermercado com %d caixas e %d clientes ===\n", N_CAIXAS, N_CLIENTES);
    sthread_init();
    // Inicializa caixas e cria funcionários
    for (int i = 0; i < N_CAIXAS; ++i) {
        caixas[i].monitor = sthread_user_monitor_init();
        caixas[i].clientes_na_fila = 0;
        caixas[i].clientes_atendidos = 0;
        sthread_create(funcionario, (void*)(long)i, 8); // prioridade média
    }
    // Cria clientes, espaçando suas chegadas
    for (int i = 0; i < N_CLIENTES; ++i) {
        sthread_create(cliente, (void*)(long)i, 5); // prioridade baixa
        sthread_user_sleep(1); // Clientes chegam em tempos diferentes
    }
    // O main pode esperar um pouco para o sistema rodar
    for (int i = 0; i < 20; ++i) {
        sthread_user_sleep(2);
        printf("[Main] Dump do estado das filas:\n");
        sthread_dump();
    }
    printf("\n=== Fim da simulação do supermercado ===\n");
    return 0;
}
