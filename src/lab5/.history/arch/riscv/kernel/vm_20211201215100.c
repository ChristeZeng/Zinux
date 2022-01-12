// arch/riscv/kernel/vm.c
#include "defs.h"
/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    //0x80000000的30到38为作为index值
    unsigned int index = 0x80000000 >> 30;
    //其中存放的是0x80000000的高于30位
    early_pgtbl[index] = (0x80000000 & 0xC0000000) | 0xE;

    unsigned int VA = 0x80000000 + PA2VA_OFFSET;
    index = VA >> 30;
    early_pgtbl[index] = (VA & 0xC0000000) | 0xE;
   
}