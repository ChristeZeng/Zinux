    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    # 1. save 32 registers and sepc to stack
    addi sp, sp, -248
    sd x1, 0(sp)
    sd x3, 8(sp)
    sd x4, 16(sp)
    sd x5, 24(sp)
    sd x6, 32(sp)
    sd x7, 40(sp)
    sd x8, 48(sp)
    sd x9, 56(sp)
    sd x10, 64(sp)
    sd x11, 72(sp)
    sd x12, 80(sp)
    sd x13, 88(sp)
    sd x14, 96(sp)
    sd x15, 104(sp)
    sd x16, 112(sp)
    sd x17, 120(sp)
    sd x18, 128(sp)
    sd x19, 136(sp)
    sd x20, 144(sp)
    sd x21, 152(sp)
    sd x22, 160(sp)
    sd x23, 168(sp)
    sd x24, 176(sp)
    sd x25, 184(sp)
    sd x26, 192(sp)
    sd x27, 200(sp)
    sd x28, 208(sp)
    sd x29, 216(sp)
    sd x30, 224(sp)
    sd x31, 232(sp)
    csrr t0, sepc
    sd t0, 240(sp)
    # 2. call trap_handler
call_handler:
    csrr a0, scause
    csrr a1, sepc
    call trap_handler

    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    ld t0, 240(sp)
    csrw sepc, t0
    ld x31, 232(sp)
    ld x30, 224(sp)
    ld x29, 216(sp)
    ld x28, 208(sp)
    ld x27, 200(sp)
    ld x26, 192(sp)
    ld x25, 184(sp)
    ld x24, 176(sp)
    ld x23, 168(sp)
    ld x22, 160(sp)
    ld x21, 152(sp)
    ld x20, 144(sp)
    ld x19, 136(sp)
    ld x18, 128(sp)
    ld x17, 120(sp)
    ld x16, 112(sp)
    ld x15, 104(sp)
    ld x14, 96(sp)
    ld x13, 88(sp)
    ld x12, 80(sp)
    ld x11, 72(sp)
    ld x10, 64(sp)
    ld x9, 56(sp)
    ld x8, 48(sp)
    ld x7, 40(sp)
    ld x6, 32(sp)
    ld x5, 24(sp)
    ld x4, 16(sp)
    ld x3, 8(sp)
    ld x1, 0(sp)    
    addi sp, sp, 248
    # 4. return from trap
    sret
    # -----------