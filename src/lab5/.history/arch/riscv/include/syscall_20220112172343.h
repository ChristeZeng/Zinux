#pragma once
#include "proc.h"
typedef unsigned long uint64;
uint64 sys_write(int fd, char *buf, uint64 count);
uint64 sys_getpid();