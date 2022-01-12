#include "syscall.h"
#include "proc.h"
#include "sbi.h"
uint64 sys_write(uint64 fd, char *buf, uint64 count) {
    for(uint64 i = 0; i < count; i++) {
        sbi_call
    }
}