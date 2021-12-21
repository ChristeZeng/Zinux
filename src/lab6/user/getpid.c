#include "syscall.h"
#include "stdio.h"


static inline long getpid() {
    long ret;
    asm volatile ("li a7, %1\n"
                  "ecall\n"
                  "mv %0, a0\n"
                : "+r" (ret) 
                : "i" (SYS_GETPID));
    return ret;
}

static inline long fork()
{
  long ret;
  asm volatile ("li a7, %1\n"
                "ecall\n"
                "mv %0, a0\n"
                : "+r" (ret) : "i" (SYS_CLONE));
  return ret;
}

int main() {

    while(1) {
        printf("[PID = %d] is running!\n",getpid());
        for (unsigned int i = 0; i < 0x4FFFFFFF; i++);
    }
}

// int main() {
//     int pid;

//     pid = fork();

//     if (pid == 0) {
//         while (1) {
//             printf("[U-CHILD] pid: %ld is running!\n", getpid());
//             for (unsigned int i = 0; i < 0x4FFFFFFF; i++);
//         } 
//     } else {
//         while (1) {
//             printf("[U-PARENT] pid: %ld is running!\n", getpid());
//             for (unsigned int i = 0; i < 0x4FFFFFFF; i++);
//         } 
//     }
//     return 0;
// }

// int main() {

//     fork();
//     fork();

//     while(1) {
//         printf("[PID = %d] is running!\n",getpid());
//         for (unsigned int i = 0; i < 0x4FFFFFFF; i++);
//     }
// }
