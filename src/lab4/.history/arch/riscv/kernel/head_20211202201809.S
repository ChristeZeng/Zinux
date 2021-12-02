#include "defs.h"

.extern start_kernel
.extern early_pgtbl
    .section .text.init
    .globl _start
_start:  
    call setup_vm
    call relocate
    #set stvec = _traps
    la t0, _traps
    csrw stvec, t0
    #set sie[STIE] = 1
    li t0, 0x20
    csrw sie, t0

    #set first time interrupt
    li t1, 10000000
    rdtime t0
    add t2, t0, t1
    mv a0, t2
    li a1, 0
    li a2, 0
    li a3, 0
    li a4, 0
    li a5, 0
    li a6, 0
    li a7, 0
    ecall
    #set sstatus[SIE] = 1
    li t0, 0x2
    csrw sstatus, t0

    # ------------------
    la sp, boot_stack_top
    # call setup_vm
    # call relocate
    call mm_init
    call setup_vm_final
    call task_init
    j start_kernel
    # ------------------

    relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t0, PA2VA_OFFSET
    add ra, ra, t0
    #add sp, sp, t0
    # set satp with early_pgtbl
    la t0, early_pgtbl
    srli t0, t0, 12
    li t1, 0x1
    slli t1, t1, 63
    or t0, t0, t1
    csrw satp, t0
    # flush tlb
    sfence.vma zero, zero

    ret
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 0x1000 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: