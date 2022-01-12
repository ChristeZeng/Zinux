#include "syscall.h"

uint64 sys_write(unsigned int fd, const char *buf, uint64 count) {
    uint64 i = 0;
    for(; i < count; i++) {
        sbi_ecall(0x1, fd, buf[i], 0, 0, 0, 0, 0);
    }
    return i;
}

extern struct task_struct* current;
uint64 sys_getpid() {
    return current->pid;
}