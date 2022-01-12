// trap.c 
#include "printk.h"
#include "clock.h"
#include "proc.h"
#include "syscall.h"

struct pt_regs {
    uint64 sstatus;
    uint64 sepc;
    uint64 x31;
    uint64 x30;
    uint64 x29;
    uint64 x28;
    uint64 x27;
    uint64 x26;
    uint64 x25;
    uint64 x24;
    uint64 x23;
    uint64 x22;
    uint64 x21;
    uint64 x20;
    uint64 x19;
    uint64 x18;
    uint64 x17;
    uint64 x16;
    uint64 x15;
    uint64 x14;
    uint64 x13;
    uint64 x12;
    uint64 x11;
    uint64 x10;
    uint64 x9;
    uint64 x8;
    uint64 x7;
    uint64 x6;
    uint64 x5;
    uint64 x4;
    uint64 x3;
    uint64 x2;
    uint64 x1;
};

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略
    if(scause == 0x8000000000000005) {
        printk("[S] Supervisor Mode Timer Interrupt\n");
        clock_set_next_event();
        do_timer();
    }
    else if(scause == 8) {
        if(regs->x17 == 64) {
            printk("[S] Supervisor Mode External Interrupt: write\n");
            regs->x10 = sys_write(regs->x10, regs->x11, regs->x12);
            regs->sepc += 4;
        }
        else if(regs->x17 == 172) {
            printk("[S] Supervisor Mode External Interrupt: get_pid()\n");
            regs->x10 = sys_getpid();
            regs->sepc += 4;
        }
        else {
            printk("[S] Unknown syscall\n");
        }
    }
}