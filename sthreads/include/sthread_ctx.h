#ifndef STHREAD_CTX_H
#define STHREAD_CTX_H 1

#include "sthread.h"

typedef struct _sthread_ctx {
    char *stackbase;
    char *sp;
} sthread_ctx_t;

typedef void (*sthread_ctx_start_func_t)(void);

sthread_ctx_t *sthread_new_ctx(sthread_ctx_start_func_t func);
sthread_ctx_t *sthread_new_blank_ctx();
void sthread_free_ctx(sthread_ctx_t *ctx);
void sthread_switch(sthread_ctx_t *old, sthread_ctx_t *new);

#endif /* STHREAD_CTX_H */
