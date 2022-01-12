#pragma once
typedef unsigned long uint64;
uint64 sys_write(unsigned int fd, const char *buf, uint64 count);
uint64 sys_getpid();