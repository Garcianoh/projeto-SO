
    .text
    .globl Xsthread_switch
    .globl Xsthread_switch_end

Xsthread_switch:
    # Save callee-saved registers and base pointer
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    # Save current stack pointer in rdi (old context)
    mov %rsp, (%rdi)

    # Load new stack pointer from rsi (new context)
    mov (%rsi), %rsp

    # Restore callee-saved registers and base pointer
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp

    ret

Xsthread_switch_end:
