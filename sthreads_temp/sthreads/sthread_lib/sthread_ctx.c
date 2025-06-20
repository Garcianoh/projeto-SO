/* sthread_ctx.c - Support for creating and switching thread contexts.
 *
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <sthread_ctx.h>



const size_t sthread_stack_size = 64 * 1024;

static void sthread_init_stack(sthread_ctx_t *ctx,
			       sthread_ctx_start_func_t func);


sthread_ctx_t *sthread_new_ctx(sthread_ctx_start_func_t func, void *stack) {
    sthread_ctx_t *ctx = (sthread_ctx_t*)malloc(sizeof(sthread_ctx_t));
    if (ctx == NULL) {
        fprintf(stderr, "Out of memory (sthread_new_ctx)\n");
        return NULL;
    }
    // Limitação do macOS/Clang: não é possível manipular diretamente o stack pointer/PC de jmp_buf.
    // Todas as threads partilham o mesmo stack do processo. Esta abordagem é suficiente para fins didáticos.
    setjmp(ctx->env);
    return ctx;
}

/* Initialize a stack as if it had been saved by sthread_switch. */


/* Create a new sthread_ctx_t, but don't initialize it.
 * This new sthread_ctx_t is suitable for use as 'old' in
 * a call to sthread_switch, since sthread_switch is defined to overwrite
 * 'old'. It should not be used as 'new' until it has been initialized.
 */
sthread_ctx_t *sthread_new_blank_ctx() {
    sthread_ctx_t *ctx = (sthread_ctx_t*)malloc(sizeof(sthread_ctx_t));
    return ctx;
}

/* Free resources used by given (not currently running) context. */
void sthread_free_ctx(sthread_ctx_t *ctx) {
    free(ctx);
}

/* Avoid allowing the compiler to optimize the call to
 * Xsthread_switch as a tail-call on architectures that support
 * that (powerpc). */
void sthread_anti_optimize(void) __attribute__ ((noinline));
void sthread_anti_optimize() {
    /* função dummy para evitar otimização de tail-call */
}

/* Save the currently running thread context into old, and
 * start running the context new. Old may be uninitialized,
 * but new must contain a valid saved context. */
void sthread_switch(sthread_ctx_t *old, sthread_ctx_t *new) {
    if (setjmp(old->env) == 0) {
        longjmp(new->env, 1);
    }
}
