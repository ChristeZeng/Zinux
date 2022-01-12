#include "printk.h"
#include "sbi.h"
extern void schedule();
extern void test();

int start_kernel() {
    printk("%d", 2021);
    printk(" Hello RISC-V\n");
    schedule();
    test(); // DO NOT DELETE !!!

	return 0;
}
