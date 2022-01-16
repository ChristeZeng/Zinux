#include "defs.h"

.altmacro
.macro STORE reg, n
    sd  \reg, (33 - \n) * 8(sp)
.endm
.macro LOOPSTORE n
    STORE x\n, \n
.endm
.macro LOAD reg, n
    ld  \reg, (33 - \n) * 8(sp)
.endm
.macro LOOPLOAD n
    LOAD x\n, \n
.endm

    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    csrr t0, sscratch
    beq t0, zero, kernel
    csrw sscratch, sp
    add t0, sp, zero
kernel:
    # 1. save 32 registers and sepc to stack
    addi sp, sp, -264
    
    .set n, 1
    .rept 31
        LOOPSTORE %n
        .set n, n + 1
    .endr

    csrr t0, sepc
    sd t0, 8(sp)
    csrr t0, sstatus
    sd t0, 0(sp) 

call_handler:
    csrr a0, scause
    csrr a1, sepc
    add a2, sp, zero
    call trap_handler

    .set n, 1
    .rept 31
        LOOPLOAD %n
        .set n, n + 1
    .endr

    ld t0, 8(sp)
    csrw sepc, t0
    ld t0, 0(sp) 
    csrw sstatus, t0
    addi sp, sp, 264

    # 4. return from trap
    csrr t0, sscratch
    beq t0, zero, _ends
    csrw sscratch, sp
    add sp, t0, zero
_ends:
    sret


    .global __dummy
__dummy:
    # YOUR CODE HERE
    csrr t0, sscratch
    csrw sscratch, sp
    add sp, t0, zero
    sret

    .globl __switch_to
__switch_to:
    # save state to prev process
    # YOUR CODE HERE
    add a0, a0, 40
    sd ra, 0(a0)
    sd sp, 8(a0)
    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)
    csrr t0, sepc
    sd t0, 112(a0)
    csrr t0, sstatus
    sd t0, 120(a0)
    csrr t0, sscratch
    sd t0, 128(a0)

    # restore state from next process
    # YOUR CODE HERE
    add a1, a1, 40
    ld ra, 0(a1)
    ld sp, 8(a1)
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)
    ld t0, 112(a1)
    csrw sepc, t0
    ld t0, 120(a1)
    csrw sstatus, t0
    ld t0, 128(a1)
    csrw sscratch, t0

    ld t0, 136(a1)
    li t1, PA2VA_OFFSET
    sub t0, t0, t1
    slli t0, t0, 20
    srli t0, t0, 32
    li t1, 0x1
    slli t1, t1, 63
    or t0, t0, t1
    csrw satp, t0
    sfence.vma zero, zero
    ret
