.extern start_kernel

    .section .text.init
    .globl _start
_start:

    #set stvec = _traps

    #set sie[STIE] = 1
    li t0, 0x20
    csrw sie, t0

    #set first time interrupt

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