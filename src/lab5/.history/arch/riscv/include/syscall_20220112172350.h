#pragma once
#include "proc.h"

uint64 sys_write(int fd, char *buf, uint64 count);
uint64 sys_getpid();