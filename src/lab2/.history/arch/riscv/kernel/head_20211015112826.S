.extern start_kernel

    .section .text.init
    .globl _start
_start:

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
    # - your code here -
    la sp, boot_stack_top
    j start_kernel
    # ------------------

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 0x1000 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: