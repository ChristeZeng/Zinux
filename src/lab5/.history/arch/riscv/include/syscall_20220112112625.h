#pragma once
typedef unsigned long uint64;
uint64 sys_write(uint64 fd, char *buf, uint64 count);
uint64 sys_getpid();