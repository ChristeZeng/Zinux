#pragma once
#include "proc.h"

extern struct task_struct* current;
uint64 sys_write(uint64 fd, char *buf, uint64 count);
uint64 sys_getpid();