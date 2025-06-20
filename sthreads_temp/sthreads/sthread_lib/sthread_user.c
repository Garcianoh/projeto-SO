/* Simplethreads Instructional Thread Package
 * 
 * sthread_user.c - Implements the sthread API using user-level threads.
 *
 *    You need to implement the routines in this file.
 *
 * Change Log:
 * 2002-04-15        rick
 *   - Initial version.
 * 2005-10-12        jccc
 *   - Added semaphores, deleted conditional variables
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <sthread.h>
#include <sthread_user.h>
#include <sthread_ctx.h>
#include <sthread_time_slice.h>
#include <sthread_user.h>
#include "queue.h"


struct _sthread {
  sthread_ctx_t *saved_ctx;
  sthread_start_func_t start_routine_ptr;
  long wake_time;
  int join_tid;
  void* join_ret;
  void* args;
  int tid;          /* meramente informativo */
  // Linux-like scheduler fields
  int prioridade_base;
  int prioridade_atual;
  int quantum_base;
  int quantum_atual;
  int quantum_por_usar;
  int nice;
  void *stack;      /* ponteiro para o stack da thread */
};


#define STHREAD_PRIO_LEVELS 15

static queue_t *active_runqueue[STHREAD_PRIO_LEVELS];   /* active runqueues by priority */
static queue_t *expired_runqueue[STHREAD_PRIO_LEVELS];  /* expired runqueues by priority */
static queue_t *dead_thr_list;        /* lista de threads "mortas" */
static queue_t *sleep_thr_list;
static queue_t *join_thr_list;
static queue_t *zombie_thr_list;
static struct _sthread *active_thr;   /* thread activa */
static int tid_gen;                   /* gerador de tid's */


#define CLOCK_TICK 100
static long Clock;


/*********************************************************************/
/* Part 1: Creating and Scheduling Threads                           */
/*********************************************************************/


void sthread_user_free(struct _sthread *thread);

void sthread_aux_start(void){
  splx(LOW);
  active_thr->start_routine_ptr(active_thr->args);
  sthread_user_exit((void*)0);
}

void sthread_user_dispatcher(void);

void sthread_user_init(void) {

  // Initialize runqueues for each priority level
  for (int i = 0; i < STHREAD_PRIO_LEVELS; ++i) {
    active_runqueue[i] = create_queue();
    expired_runqueue[i] = create_queue();
  }
  dead_thr_list = create_queue();
  sleep_thr_list = create_queue();
  join_thr_list = create_queue();
  zombie_thr_list = create_queue();
  tid_gen = 1;

  struct _sthread *main_thread = malloc(sizeof(struct _sthread));
  main_thread->start_routine_ptr = NULL;
  main_thread->args = NULL;
  main_thread->saved_ctx = sthread_new_blank_ctx();
  main_thread->wake_time = 0;
  main_thread->join_tid = 0;
  main_thread->join_ret = NULL;
  main_thread->tid = tid_gen++;
  // Linux-like scheduler initialization for main thread
  main_thread->prioridade_base = 0;
  main_thread->prioridade_atual = 0;
  main_thread->quantum_base = 5;
  main_thread->quantum_atual = 5;
  main_thread->quantum_por_usar = 5;
  main_thread->nice = 0;
  
  active_thr = main_thread;

  // Insert main thread into the highest-priority active runqueue (priority 0)
  queue_insert(active_runqueue[0], main_thread);

  Clock = 1;
  sthread_time_slices_init(sthread_user_dispatcher, CLOCK_TICK);
}


sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg, int priority)
{
  struct _sthread *new_thread = (struct _sthread*)malloc(sizeof(struct _sthread));
  sthread_ctx_start_func_t func = sthread_aux_start;
  new_thread->args = arg;
  new_thread->start_routine_ptr = start_routine;
  new_thread->wake_time = 0;
  new_thread->join_tid = 0;
  new_thread->join_ret = NULL;
  new_thread->stack = malloc(64*1024); // 64KB stack por thread
  new_thread->saved_ctx = sthread_new_ctx(func, new_thread->stack);

  // Linux-like scheduler initialization
  new_thread->prioridade_base = priority;
  new_thread->prioridade_atual = priority;
  new_thread->quantum_base = 5; // as specified
  new_thread->quantum_atual = 5;
  new_thread->quantum_por_usar = 5;
  new_thread->nice = 0;

  splx(HIGH);
  new_thread->tid = tid_gen++;
  // Insert into the correct active runqueue based on priority
  int prio = new_thread->prioridade_atual;
  if (prio < 0) prio = 0;
  if (prio >= STHREAD_PRIO_LEVELS) prio = STHREAD_PRIO_LEVELS - 1;
  queue_insert(active_runqueue[prio], new_thread);
  splx(LOW);
  return new_thread;
}


void sthread_user_exit(void *ret) {
  splx(HIGH);
   
   int is_zombie = 1;

   // 1. Desbloqueia threads esperando por join
   queue_t *tmp_queue = create_queue();   
   while (!queue_is_empty(join_thr_list)) {
      struct _sthread *thread = queue_remove(join_thr_list);
     
      printf("Test join list: join_tid=%d, active->tid=%d\n", thread->join_tid, active_thr->tid);
      if (thread->join_tid == active_thr->tid) {
         thread->join_ret = ret;
         // Insere a thread desbloqueada na runqueue de sua prioridade
         int prio = thread->prioridade_atual;
         if (prio < 0) prio = 0;
         if (prio >= STHREAD_PRIO_LEVELS) prio = STHREAD_PRIO_LEVELS - 1;
         queue_insert(active_runqueue[prio], thread);
         is_zombie = 0;
      } else {
         queue_insert(tmp_queue,thread);
      }
   }
   delete_queue(join_thr_list);
   join_thr_list = tmp_queue;
  
   // 2. Move a thread atual para a lista apropriada (zumbi ou mortas)
   if (is_zombie) {
      queue_insert(zombie_thr_list, active_thr);
   } else {
      queue_insert(dead_thr_list, active_thr);
   }
   
   // 3. Se não há mais threads executáveis, encerra o programa
   int runq_empty = 1;
   for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
     if (!queue_is_empty(active_runqueue[p])) {
       runq_empty = 0;
       break;
     }
   }
   if (runq_empty) {
    delete_queue(dead_thr_list);
    sthread_user_free(active_thr);
    printf("Runqueue is empty!\n");
    exit(0);
  }

   // 4. Troca de contexto para próxima thread da runqueue
   struct _sthread *old_thr = active_thr;
   struct _sthread *next_thr = NULL;
   for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
     if (!queue_is_empty(active_runqueue[p])) {
       next_thr = queue_remove(active_runqueue[p]);
       break;
     }
   }
   if (!next_thr) {
     // Swap de epoch se necessário
     for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
       queue_t *tmp = active_runqueue[p];
       active_runqueue[p] = expired_runqueue[p];
       expired_runqueue[p] = tmp;
     }
     for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
       if (!queue_is_empty(active_runqueue[p])) {
         next_thr = queue_remove(active_runqueue[p]);
         break;
       }
     }
   }
   if (!next_thr) {
     printf("Nenhuma thread pronta para executar!\n");
     exit(0);
   }
   active_thr = next_thr;
   sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);

   splx(LOW);
}


void sthread_user_dispatcher(void)
{
   Clock++;

   // Cria uma fila temporária para threads que ainda não devem acordar
   queue_t *tmp_queue = create_queue();   

   // 1. Percorre todas as threads adormecidas
   while (!queue_is_empty(sleep_thr_list)) {
      struct _sthread *thread = queue_remove(sleep_thr_list);
      
      // 2. Se chegou a hora de acordar, insere na runqueue de sua prioridade
      if (thread->wake_time == Clock) {
         thread->wake_time = 0;
         int prio = thread->prioridade_atual;
         if (prio < 0) prio = 0;
         if (prio >= STHREAD_PRIO_LEVELS) prio = STHREAD_PRIO_LEVELS - 1;
         queue_insert(active_runqueue[prio], thread);
      } else {
         // 3. Se ainda não é hora, mantém na fila de sleep
         queue_insert(tmp_queue,thread);
      }
   }
   // Substitui a fila de sleep pela temporária
   delete_queue(sleep_thr_list);
   sleep_thr_list = tmp_queue;
   
   // 4. Chama yield para continuar o escalonamento
   sthread_user_yield();
}



void sthread_user_yield(void)
{
  splx(HIGH);
  struct _sthread *old_thr = active_thr;

  // 1. Decrementa quantum da thread atual
  old_thr->quantum_por_usar--;

  // 2. Se o quantum esgotou, move a thread para expired_runqueue
  if (old_thr->quantum_por_usar <= 0) {
    // Recalcula quantum/prioridade para o próximo epoch
    old_thr->quantum_por_usar = old_thr->quantum_base; // Pode-se ajustar para quantum dinâmico
    int new_prio = old_thr->prioridade_base + old_thr->nice;
    if (new_prio < 0) new_prio = 0;
    if (new_prio >= STHREAD_PRIO_LEVELS) new_prio = STHREAD_PRIO_LEVELS - 1;
    old_thr->prioridade_atual = new_prio;
    queue_insert(expired_runqueue[old_thr->prioridade_atual], old_thr);
  } else {
    // Se ainda tem quantum, volta para a runqueue ativa
    int prio = old_thr->prioridade_atual;
    if (prio < 0) prio = 0;
    if (prio >= STHREAD_PRIO_LEVELS) prio = STHREAD_PRIO_LEVELS - 1;
    queue_insert(active_runqueue[prio], old_thr);
  }

  // 3. Seleciona a próxima thread de maior prioridade disponível
  struct _sthread *next_thr = NULL;
  for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
    if (!queue_is_empty(active_runqueue[p])) {
      next_thr = queue_remove(active_runqueue[p]);
      break;
    }
  }

  // 4. Se não há nenhuma thread na runqueue ativa, faz swap de epoch
  if (!next_thr) {
    // Troca active <-> expired
    for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
      queue_t *tmp = active_runqueue[p];
      active_runqueue[p] = expired_runqueue[p];
      expired_runqueue[p] = tmp;
    }
    // Busca novamente a próxima thread
    for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
      if (!queue_is_empty(active_runqueue[p])) {
        next_thr = queue_remove(active_runqueue[p]);
        break;
      }
    }
  }

  // 5. Se ainda não há thread (caso raro), mantém a thread atual
  if (!next_thr) {
    next_thr = old_thr;
  }

  active_thr = next_thr;
  sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
  splx(LOW);
}





void sthread_user_free(struct _sthread *thread)
{
  sthread_free_ctx(thread->saved_ctx);
  if (thread->stack) free(thread->stack);
  free(thread);
}


/*********************************************************************/
/* Part 2: Join and Sleep Primitives                                 */
/*********************************************************************/

int sthread_user_join(sthread_t thread, void **value_ptr)
{
   /* suspends execution of the calling thread until the target thread
      terminates, unless the target thread has already terminated.
      On return from a successful pthread_join() call with a non-NULL 
      value_ptr argument, the value passed to pthread_exit() by the 
      terminating thread is made available in the location referenced 
      by value_ptr. When a pthread_join() returns successfully, the 
      target thread has been terminated. The results of multiple 
      simultaneous calls to pthread_join() specifying the same target 
      thread are undefined. If the thread calling pthread_join() is 
      canceled, then the target thread will not be detached. 

      If successful, the pthread_join() function returns zero. 
      Otherwise, an error number is returned to indicate the error. */

   
   splx(HIGH);
   // checks if the thread to wait is zombie
   int found = 0;
   queue_t *tmp_queue = create_queue();
   while (!queue_is_empty(zombie_thr_list)) {
      struct _sthread *zthread = queue_remove(zombie_thr_list);
      if (thread->tid == zthread->tid) {
         *value_ptr = thread->join_ret;
         queue_insert(dead_thr_list,thread);
         found = 1;
      } else {
         queue_insert(tmp_queue,zthread);
      }
   }
   delete_queue(zombie_thr_list);
   zombie_thr_list = tmp_queue;
  
   if (found) {
       splx(LOW);
       return 0;
   }

   
   // search active queue
   if (active_thr->tid == thread->tid) {
      found = 1;
   }
   
   queue_element_t *qe = NULL;

   // search exe
   // Adaptação: percorra a runqueue ativa de todas as prioridades
   qe = NULL;
   for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
     if (!queue_is_empty(active_runqueue[p])) {
       qe = active_runqueue[p]->first;
       break;
     }
   }
   while (!found && qe != NULL) {
      if (qe->thread->tid == thread->tid) {
         printf("Found in exe: tid=%d\n", thread->tid);
         found = 1;
      }
      qe = qe->next;
   }

   // search sleep
   qe = sleep_thr_list->first;
   while (!found && qe != NULL) {
      if (qe->thread->tid == thread->tid) {
         found = 1;
      }
      qe = qe->next;
   }

   // search join
   qe = join_thr_list->first;
   while (!found && qe != NULL) {
      if (qe->thread->tid == thread->tid) {
         found = 1;
      }
      qe = qe->next;
   }

   // if found blocks until thread ends
   if (!found) {
      splx(LOW);
      return -1;
   } else {
      active_thr->join_tid = thread->tid;
      
      struct _sthread *old_thr = active_thr;
      queue_insert(join_thr_list, old_thr);
      // Adaptação: seleciona próxima thread de maior prioridade disponível
      struct _sthread *next_thr = NULL;
      for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
        if (!queue_is_empty(active_runqueue[p])) {
          next_thr = queue_remove(active_runqueue[p]);
          break;
        }
      }
      if (!next_thr) {
        // Swap de epoch se necessário
        for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
          queue_t *tmp = active_runqueue[p];
          active_runqueue[p] = expired_runqueue[p];
          expired_runqueue[p] = tmp;
        }
        for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
          if (!queue_is_empty(active_runqueue[p])) {
            next_thr = queue_remove(active_runqueue[p]);
            break;
          }
        }
      }
      if (!next_thr) {
        printf("Nenhuma thread pronta para executar!\n");
        exit(0);
      }
      active_thr = next_thr;
      printf ("Active is 0:%d\n", (active_thr == NULL));
      printf ("Old is 0:%d\n", (old_thr == NULL));
      sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
  
      *value_ptr = thread->join_ret;
   }
   
   splx(LOW);
   return 0;
}


int sthread_user_sleep(int time)
{
   splx(HIGH);
   
   // Calcula quantos ticks a thread deve dormir
   long num_ticks = 10 * time / CLOCK_TICK;
   if (num_ticks == 0) {
      splx(LOW);
      return 0;
   }
   
   // Define o tempo de acordar da thread
   active_thr->wake_time = Clock + num_ticks;

   // Insere a thread na fila de sleep
   queue_insert(sleep_thr_list, active_thr);

   // Salva ponteiro para a thread atual
   sthread_t old_thr = active_thr;

   // Chama yield para trocar de contexto e escalonar outra thread
   sthread_user_yield();

   splx(LOW);
   return 0;
}

/* --------------------------------------------------------------------------*
 * Synchronization Primitives                                                *
 * ------------------------------------------------------------------------- */

/*
 * Mutex implementation
 */

struct _sthread_mutex
{
  lock_t l;
  struct _sthread *thr;
  queue_t* queue;
};

sthread_mutex_t sthread_user_mutex_init()
{
  sthread_mutex_t lock;

  if(!(lock = malloc(sizeof(struct _sthread_mutex)))){
    printf("Error in creating mutex\n");
    return 0;
  }

  /* mutex initialization */
  lock->l=0;
  lock->thr = NULL;
  lock->queue = create_queue();
  
  return lock;
}

void sthread_user_mutex_free(sthread_mutex_t lock)
{
  delete_queue(lock->queue);
  free(lock);
}

void sthread_user_mutex_lock(sthread_mutex_t lock)
{
  while(atomic_test_and_set(&(lock->l))) {}

  if(lock->thr == NULL){
    lock->thr = active_thr;

    atomic_clear(&(lock->l));
  } else {
    queue_insert(lock->queue, active_thr);
    
    atomic_clear(&(lock->l));

    splx(HIGH);
    struct _sthread *old_thr;
    old_thr = active_thr;
    // Não insere mais em exe_thr_list; uso removido pelo novo escalonador
    // Removido: troca de contexto feita por sthread_user_yield()

    sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);

    splx(LOW);
  }
}

void sthread_user_mutex_unlock(sthread_mutex_t lock)
{
  if(lock->thr!=active_thr){
    printf("unlock without lock!\n");
    return;
  }

  while(atomic_test_and_set(&(lock->l))) {}

  if(queue_is_empty(lock->queue)){
    lock->thr = NULL;
  } else {
    lock->thr = queue_remove(lock->queue);
    // Adaptação: insere a thread desbloqueada na runqueue de sua prioridade
    int prio = lock->thr->prioridade_atual;
    if (prio < 0) prio = 0;
    if (prio >= STHREAD_PRIO_LEVELS) prio = STHREAD_PRIO_LEVELS - 1;
    queue_insert(active_runqueue[prio], lock->thr);
  }

  atomic_clear(&(lock->l));
}

/*
 * Ajusta o valor nice da thread atual e retorna a nova prioridade calculada
 */
int sthread_user_nice(int nice) {
    active_thr->nice = nice;
    // Calcula nova prioridade dinâmica para o próximo epoch
    int new_prio = active_thr->prioridade_base + nice;
    if (new_prio < 0) new_prio = 0;
    if (new_prio >= STHREAD_PRIO_LEVELS) new_prio = STHREAD_PRIO_LEVELS - 1;
    active_thr->prioridade_atual = new_prio;
    return new_prio;
}

/*
 * Monitor implementation
 */

struct _sthread_mon {
 	sthread_mutex_t mutex;
	queue_t* queue;
};

/*
 * sthread_dump - Exibe o estado das filas de execução, threads adormecidas, zumbis e mortas.
 * Útil para depuração visual do escalonador Linux-like O(1).
 */
void sthread_user_dump(void) {
    printf("\n================= SCHEDULER DUMP =================\n");
    // Exibe filas ativas por prioridade
    for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
        printf("[PRIO %2d] Active: ", p);
        queue_element_t *qe = active_runqueue[p]->first;
        while (qe) {
            struct _sthread *t = qe->thread;
            printf("[TID=%d prio=%d q=%d nice=%d] ", t->tid, t->prioridade_atual, t->quantum_por_usar, t->nice);
            qe = qe->next;
        }
        printf("\n");
    }
    // Exibe filas expiradas por prioridade
    for (int p = 0; p < STHREAD_PRIO_LEVELS; ++p) {
        printf("[PRIO %2d] Expired: ", p);
        queue_element_t *qe = expired_runqueue[p]->first;
        while (qe) {
            struct _sthread *t = qe->thread;
            printf("[TID=%d prio=%d q=%d nice=%d] ", t->tid, t->prioridade_atual, t->quantum_por_usar, t->nice);
            qe = qe->next;
        }
        printf("\n");
    }
    // Exibe threads adormecidas
    printf("Sleeping: ");
    queue_element_t *qe = sleep_thr_list->first;
    while (qe) {
        struct _sthread *t = qe->thread;
        printf("[TID=%d prio=%d wake=%ld] ", t->tid, t->prioridade_atual, t->wake_time);
        qe = qe->next;
    }
    printf("\n");
    // Exibe threads zumbis
    printf("Zombies: ");
    qe = zombie_thr_list->first;
    while (qe) {
        struct _sthread *t = qe->thread;
        printf("[TID=%d prio=%d] ", t->tid, t->prioridade_atual);
        qe = qe->next;
    }
    printf("\n");
    // Exibe threads mortas
    printf("Dead: ");
    qe = dead_thr_list->first;
    while (qe) {
        struct _sthread *t = qe->thread;
        printf("[TID=%d prio=%d] ", t->tid, t->prioridade_atual);
        qe = qe->next;
    }
    printf("\n");
    // Exibe thread ativa
    if (active_thr) {
        printf("Active thread: [TID=%d prio=%d q=%d nice=%d]\n", active_thr->tid, active_thr->prioridade_atual, active_thr->quantum_por_usar, active_thr->nice);
    } else {
        printf("Active thread: NULL\n");
    }
    printf("==================================================\n\n");
}

sthread_mon_t sthread_user_monitor_init()
{
  sthread_mon_t mon;
  if(!(mon = malloc(sizeof(struct _sthread_mon)))){
    printf("Error creating monitor\n");
    return 0;
  }

  mon->mutex = sthread_user_mutex_init();
  mon->queue = create_queue();
  return mon;
}

void sthread_user_monitor_free(sthread_mon_t mon)
{
  sthread_user_mutex_free(mon->mutex);
  delete_queue(mon->queue);
  free(mon);
}

void sthread_user_monitor_enter(sthread_mon_t mon)
{
  sthread_user_mutex_lock(mon->mutex);
}

void sthread_user_monitor_exit(sthread_mon_t mon)
{
  sthread_user_mutex_unlock(mon->mutex);
}

void sthread_user_monitor_wait(sthread_mon_t mon)
{
  struct _sthread *temp;

  // Verifica se a thread atual realmente possui o mutex
  if(mon->mutex->thr != active_thr){
    printf("monitor wait called outside monitor\n");
    return;
  }

  // 1. Insere a thread atual na fila de bloqueados do monitor
  temp = active_thr;
  queue_insert(mon->queue, temp);

  // 2. Sai da região de exclusão mútua
  sthread_user_mutex_unlock(mon->mutex);

  // 3. Chama yield para remover a thread do escalonamento até ser sinalizada
  sthread_user_yield();
}

void sthread_user_monitor_signal(sthread_mon_t mon)
{
  struct _sthread *temp;

  // Verifica se a thread atual realmente possui o mutex
  if(mon->mutex->thr != active_thr){
    printf("monitor signal called outside monitor\n");
    return;
  }

  // 1. Entra em região crítica para manipular a fila do monitor
  while(atomic_test_and_set(&(mon->mutex->l))) {}

  // 2. Se há threads bloqueadas no monitor, remove uma
  if(!queue_is_empty(mon->queue)){
    temp = queue_remove(mon->queue);
    // 3. Insere a thread desbloqueada diretamente na runqueue da sua prioridade
    int prio = temp->prioridade_atual;
    if (prio < 0) prio = 0;
    if (prio >= STHREAD_PRIO_LEVELS) prio = STHREAD_PRIO_LEVELS - 1;
    queue_insert(active_runqueue[prio], temp);
  }

  // 4. Sai da região crítica
  atomic_clear(&(mon->mutex->l));
}




/* The following functions are dummies to 
 * highlight the fact that pthreads do not
 * include monitors.
 */

sthread_mon_t sthread_dummy_monitor_init()
{
   printf("WARNING: pthreads do not include monitors!\n");
   return NULL;
}


void sthread_dummy_monitor_free(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_enter(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_exit(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_wait(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_signal(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}



