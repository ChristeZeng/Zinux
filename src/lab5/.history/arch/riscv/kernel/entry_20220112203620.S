#include "defs.h"

.macro SAVE_X_SP reg, offset
    sd  \reg, \offset*8(sp)
.endm

.macro SAVE_X_SP_N n
    SAVE_X_SP    x\n, \n
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
    
    .set    n, 2
    .rept   29
        SAVE_X_SP_N  %n
        .set    n, n + 1
    .endr
    
    # sd x1, 256(sp)
    # sd x2, 248(sp)
    # sd x3, 240(sp)
    # sd x4, 232(sp)
    # sd x5, 224(sp)
    # sd x6, 216(sp)
    # sd x7, 208(sp)
    # sd x8, 200(sp)
    # sd x9, 192(sp)
    # sd x10, 184(sp)
    # sd x11, 176(sp)
    # sd x12, 168(sp)
    # sd x13, 160(sp)
    # sd x14, 152(sp)
    # sd x15, 144(sp)
    # sd x16, 136(sp)
    # sd x17, 128(sp)
    # sd x18, 120(sp)
    # sd x19, 112(sp)
    # sd x20, 104(sp)
    # sd x21, 96(sp)
    # sd x22, 88(sp)
    # sd x23, 80(sp)
    # sd x24, 72(sp)
    # sd x25, 64(sp)
    # sd x26, 56(sp)
    # sd x27, 48(sp)
    # sd x28, 40(sp)
    # sd x29, 32(sp)
    # sd x30, 24(sp)
    # sd x31, 16(sp)
    csrr t0, sepc
    sd t0, 8(sp)
    csrr t0, sstatus
    sd t0, 0(sp) 

call_handler:
    csrr a0, scause
    csrr a1, sepc
    add a2, sp, zero
    call trap_handler

    ld x1, 256(sp)
    ld x2, 248(sp)
    ld x3, 240(sp)
    ld x4, 232(sp)
    ld x5, 224(sp)
    ld x6, 216(sp)
    ld x7, 208(sp)
    ld x8, 200(sp)
    ld x9, 192(sp)
    ld x10, 184(sp)
    ld x11, 176(sp)
    ld x12, 168(sp)
    ld x13, 160(sp)
    ld x14, 152(sp)
    ld x15, 144(sp)
    ld x16, 136(sp)
    ld x17, 128(sp)
    ld x18, 120(sp)
    ld x19, 112(sp)
    ld x20, 104(sp)
    ld x21, 96(sp)
    ld x22, 88(sp)
    ld x23, 80(sp)
    ld x24, 72(sp)
    ld x25, 64(sp)
    ld x26, 56(sp)
    ld x27, 48(sp)
    ld x28, 40(sp)
    ld x29, 32(sp)
    ld x30, 24(sp)
    ld x31, 16(sp)
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
