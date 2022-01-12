#include "syscall.h"
extern struct task_struct* current;
uint64 sys_write(unsigned int fd, const char *buf, uint64 count) {
    uint64 i = 0;
    for(; i < count; i++) {
        sbi_ecall(0x1, fd, (const char *)buf[i], 0, 0, 0, 0, 0);
    }
    return i;
}

uint64 sys_getpid() {
    return current->pid;
}