#include "print.h"
#include "sbi.h"

extern void test();

int start_kernel() {
    puti(2021);
    puts(" Hello RISC-V\n");

    test(); // DO NOT DELETE !!!

	return 0;
}
