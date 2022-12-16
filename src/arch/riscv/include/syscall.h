#pragma once
#include "proc.h"

uint64 sys_write(unsigned int fd, const char* buf, uint64 count);
uint64 sys_getpid();