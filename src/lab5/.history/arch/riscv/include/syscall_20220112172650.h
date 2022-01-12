#pragma once
#include "proc.h"

uint64 sys_write(unsigned int fd, const char* buf, size_t count);
uint64 sys_getpid();