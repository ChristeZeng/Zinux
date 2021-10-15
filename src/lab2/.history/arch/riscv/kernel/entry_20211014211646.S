    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    # 1. save 32 registers and sepc to stack
    addi sp, sp, -256
    sd x1, 0(sp)
    sd x2, 8(sp)
    

    # -----------

        # 2. call trap_handler

    # -----------

        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack

    # -----------

        # 4. return from trap

    # -----------