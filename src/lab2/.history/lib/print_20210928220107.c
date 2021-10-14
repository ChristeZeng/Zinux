#include "print.h"
#include "sbi.h"

void puts(char *s) {
    while(*s != 0)
    {
        sbi_ecall(0x1, 0x0, *s, 0, 0, 0, 0, 0);
        s++;
    }
    // unimplemented
}

void puti(int x) {

    int bit[100];
    int cnt = 0;
    while(x)
    {
        bit[cnt++] = x % 10;
        x /= 10;
    }
    for(int i = cnt - 1; i >= 0; i--)
        sbi_ecall(0x1, 0x0, bit[i]+'0', 0, 0, 0, 0, 0);
}
