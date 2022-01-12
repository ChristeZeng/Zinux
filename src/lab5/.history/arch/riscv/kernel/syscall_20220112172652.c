#include "syscall.h"
#include "sbi.h"

extern struct task_struct* current;
sys_write(unsigned int fd, const char* buf, size_t count) {
    uint64 i = 0;
    for(; i < count; i++) {
        sbi_ecall(0x1, fd, (uint64)buf[i], 0, 0, 0, 0, 0);
    }
    return i;
}

uint64 sys_getpid() {
    return current->pid;
}