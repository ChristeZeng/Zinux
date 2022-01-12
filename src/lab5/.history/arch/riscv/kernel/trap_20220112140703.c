// trap.c 
#include "printk.h"
#include "clock.h"
#include "proc.h"
#include "syscall.h"

struct pt_regs {
    uint64 sepc;
    uint64 x[31];
    uint64 sstatus;
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
        if(regs->x[16] == 64) {
            regs->x[9] = sys_write(regs->x[9], (char*)regs->x[10], regs->x[11]);
            regs->sepc += 4;
        }
        else if(regs->x[16] == 172) {
            regs->x[9] = sys_getpid();
            regs->sepc += 4;
        }
        else {
            printk("[S] Unknown syscall\n");
        }
    }
}