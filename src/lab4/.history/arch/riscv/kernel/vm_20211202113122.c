// arch/riscv/kernel/vm.c
#include "defs.h"
#include "types.h"
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
    unsigned long index = (PHY_START & 0x0000007FC0000000) >> 30;
    //其中存放的是0x80000000的高于30位
    early_pgtbl[index] = ((PHY_START & 0xFFFFFFFFFFFFF000) << 16) | 0xF;

    unsigned long VA = PHY_START + PA2VA_OFFSET;
    index = (VA & 0x0000007FC0000000) >> 30;
    early_pgtbl[index] = ((VA & 0xFFFFFFFFFFFFF000) << 16)| 0xF;
   
}

// arch/riscv/kernel/vm.c 

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
// unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

// void setup_vm_final(void) {
//     memset(swapper_pg_dir, 0x0, PGSIZE);

//     // No OpenSBI mapping required

//     // mapping kernel text X|-|R|V
//     create_mapping(...);

//     // mapping kernel rodata -|-|R|V
//     create_mapping(...);

//     // mapping other memory -|W|R|V
//     create_mapping(...);

//     // set satp with swapper_pg_dir

//     YOUR CODE HERE

//     // flush TLB
//     asm volatile("sfence.vma zero, zero");
//     return;
// }


// /* 创建多级页表映射关系 */
// create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
//     /*
//     pgtbl 为根页表的基地址
//     va, pa 为需要映射的虚拟地址、物理地址
//     sz 为映射的大小
//     perm 为映射的读写权限

//     创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
//     可以使用 V bit 来判断页表项是否存在
//     */
//    int page_num = (page_num % 0x1000) ? ((sz >> 12) + 1) : (sz >> 12);
//    for(int i = 0; i < page_num; i++) {
//         //取出va + i * 0x1000的[38, 30]位作为vpn2
//         uint64 vpn2 = (va + i * 0x1000) & 0x0000007FC0000000;
//         //取出va + i * 0x1000的[29, 21]位作为vpn1
//         uint64 vpn1 = (va + i * 0x1000) & 0x000000003FE00000;
//         //取出va + i * 0x1000的[20, 12]位作为vpn0
//         uint64 vpn0 = (va + i * 0x1000) & 0x00000000001FF000;

//         //取出pa + i * 0x1000的[55, 30]位作为ppn2
//         uint64 ppn2 = (pa + i * 0x1000) & 0x00FFFFFFC0000000;
//         //取出pa + i * 0x1000的[29, 21]位作为ppn1
//         uint64 ppn1 = (pa + i * 0x1000) & 0x000000003FE00000;
//         //取出pa + i * 0x1000的[20, 12]位作为ppn0
//         uint64 ppn0 = (pa + i * 0x1000) & 0x00000000001FF000;

//         uint64 first_ptl_addr = (uint64)pgtbl + (vpn2 << 3);
//         uint64 first_pte = *(uint64 *)first_ptl_addr;
//         if(!(first_pte & 0x1)) {
//             //如果页表项不存在，则创建页表项
//             uint64 content = (uint64*)kalloc();
//             first_pte
            
//         }


//    }
// }