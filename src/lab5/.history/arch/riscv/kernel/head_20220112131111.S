.extern start_kernel

    .section .text.init
    .globl _start
    
_start:
    la sp, boot_stack_top
    addi sp, sp, -16
    sd fp, 0(sp)
    sd ra, 8(sp)

    jal ra, setup_vm
    jal ra, relocate
_relocate_done:

    addi fp, sp, 0

    jal ra, mm_init
_mem_init_done:

    jal ra, setup_vm_final

    la t0, _traps
    csrrw t1, stvec, t0


    addi t0, zero, 32
    csrrs t0, sie, t0

    
    jal ra, clock_set_next_event

    # csrrsi t0, sstatus, 2

    jal ra, task_init

    jal ra, start_kernel

    ld fp, 0(sp)
    ld ra, 8(sp)
    addi sp, sp, 16
    jalr x0, ra, 0
    
relocate:
    # li t0, PA2VA_OFFSET
    li t0, 0xffffffdf80000000
    add ra, ra, t0;
    add sp, sp ,t0;

    la t0, early_pgtbl
    srl t0, t0, 12
    li t1, 0x8000000000000000
    or t0, t0, t1
    csrw satp, t0
    sfence.vma zero, zero
    fence.i
    ret

    .section .bss.stack
    .globl boot_stack

boot_stack:
    .space 0x1000
    .globl boot_stack_top

boot_stack_top:
