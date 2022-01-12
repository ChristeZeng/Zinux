    .section .text.entry
    .align 2
    .globl _traps 
_traps:

    # finish this in the user space stack
    addi sp, sp, -16
    sd t0, 8(sp)
    sd t1, 0(sp)
    csrr t0, sscratch
    beq t0, zero, is_kernel_thread
    # 0(sp) = t1
    # 8(sp) = t0
    # 16(sp) = sscratch
is_user_thread:
    csrr t0, sscratch
    addi t0, t0, -8
    # store the original sp to the kernel space

    addi sp, sp, 16
    sd sp, 0(t0)
    # sp = kernel_sp - 8
    # kernel_sp[0] = user_sp
    add sp, t0, zero
    # here t0 is original sp
    ld t0, 0(t0)
    addi t0, t0, -16
    ld t1, 8(t0)
    csrw sscratch, t1
    ld t1, 0(t0)
    csrr t0, sscratch
    # make the scratch zero
    csrwi sscratch, 0
    beq zero, zero, space_change_done

is_kernel_thread:
    # as if nothing ever happens
    # just get the stack raised

    ld t0, 8(sp)
    li t1, 0
    sd t1, 8(sp)
    ld t1, 0(sp)
    addi sp, sp, 8

space_change_done:
    
    addi sp, sp, -264
    sd x1, 248(sp)
    csrr x1, sstatus
    sd x1, 256(sp)
    sd x2, 240(sp)
    sd x3, 232(sp)
    sd x4, 224(sp)
    sd x5, 216(sp)
    sd x6, 208(sp)
    sd x7, 200(sp)
    sd x8, 192(sp)
    sd x9, 184(sp)
    sd x10, 176(sp)
    sd x11, 168(sp)
    sd x12, 160(sp)
    sd x13, 152(sp)
    sd x14, 144(sp)
    sd x15, 136(sp)
    sd x16, 128(sp)
    sd x17, 120(sp)
    sd x18, 112(sp)
    sd x19, 104(sp)
    sd x20, 96(sp)
    sd x21, 88(sp)
    sd x22, 80(sp)
    sd x23, 72(sp)
    sd x24, 64(sp)
    sd x25, 56(sp)
    sd x26, 48(sp)
    sd x27, 40(sp)
    sd x28, 32(sp)
    sd x29, 24(sp)
    sd x30, 16(sp)
    sd x31, 8(sp)
    csrr t0, sepc
    sd t0, 0(sp) 

    csrr a0, scause
    csrr a1, sepc
    add a2, sp, zero
    jal x1, trap_handler

    ld x1, 256(sp)
    csrw sstatus, x1
    ld x1, 248(sp)
    ld x2, 240(sp)
    ld x3, 232(sp)
    ld x4, 224(sp)
    ld x5, 216(sp)
    ld x6, 208(sp)
    ld x7, 200(sp)
    ld x8, 192(sp)
    ld x9, 184(sp)
    ld x10, 176(sp)
    ld x11, 168(sp)
    ld x12, 160(sp)
    ld x13, 152(sp)
    ld x14, 144(sp)
    ld x15, 136(sp)
    ld x16, 128(sp)
    ld x17, 120(sp)
    ld x18, 112(sp)
    ld x19, 104(sp)
    ld x20, 96(sp)
    ld x21, 88(sp)
    ld x22, 80(sp)
    ld x23, 72(sp)
    ld x24, 64(sp)
    ld x25, 56(sp)
    ld x26, 48(sp)
    ld x27, 40(sp)
    ld x28, 32(sp)
    ld x29, 24(sp)
    ld x30, 16(sp)
    ld x31, 8(sp)
    ld t0, 0(sp)
    csrw sepc, t0
    addi sp, sp, 264

    addi sp, sp, -16
    sd t0, 0(sp)
    sd t1, 8(sp)
    ld t0, 16(sp)
    add t1, sp, zero
    beq t0, zero, it_was_kernel

it_was_user:
    csrw sscratch, t0
    addi sp, sp, 24
    csrrw sp, sscratch, sp
    ld t0, 0(t1)
    ld t1, 8(t1)
    beq zero, zero, space_change_back_done

it_was_kernel:
    ld t0, 0(sp)
    ld t1, 8(sp)
    addi sp, sp, 24
space_change_back_done:

    sret

    .globl __dummy
__dummy:
    #la t0, dummy 
    #li t0, 0
    # la t0, main
    #csrw sepc, t0
    csrr t0, sscratch
    csrw sscratch, sp
    add sp, t0, zero
    # csrrw sp, sscratch, sp
    sret

    .globl __switch_to
__switch_to:
    addi a0, a0, 40
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

    csrr s11, sepc
    sd s11, 112(a0)
    csrr s11, sstatus
    sd s11, 120(a0)
    csrr s11, sscratch
    sd s11, 128(a0)

    addi a1, a1, 40
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

    ld s11, 112(a1)
    csrw sepc, s11
    ld s11, 120(a1)
    csrw sstatus, s11
    ld s11, 128(a1)
    csrw sscratch, s11

    ld s11, 136(a1)
    li s10, 0xffffffdf80000000
    sub s11, s11, s10
    slli s11, s11, 20
    srli s11, s11, 32
    li s10, 0x8000000000000000
    or s11, s11, s10
    csrw satp, s11
    sfence.vma zero, zero
    fence.i

    ld s10, 96(a1)
    ld s11, 104(a1)
    ret