#include "syscall.h"
#include "proc.h"

uint64 sys_write(uint64 fd, char *buf, uint64 count) {
    if(fd == 1) {
        printk("%s", buf);
        return count;
    }
    return 0;
}