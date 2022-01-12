// arch/riscv/kernel/vm.c
#include "defs.h"
#include "types.h"
#include "string.h"
#include "printk.h"
#include "mm.h"

extern char _srodata[];
extern char _stext[];
extern char _sdata[];
extern char _sbss[];
extern char _ekernel[];

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
    uint64 index = ((uint64)PHY_START & 0x0000007FC0000000) >> 30;
    early_pgtbl[index] = ((PHY_START & 0x00FFFFFFC0000000) >> 2 ) | 0xF;

    uint64 VA = PHY_START + PA2VA_OFFSET;
    index = (VA & 0x0000007FC0000000) >> 30;
    early_pgtbl[index] = ((PHY_START & 0x00FFFFFFC0000000) >> 2) | 0xF;
    printk("setup_vm done!\n");
}

// arch/riscv/kernel/vm.c 

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

/* 创建多级页表映射关系 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
   //printk("\ncreate_mapping: pgtbl: %lx, va: %lx, pa: %lx, sz: %lx, perm: %d\n", pgtbl, va, pa, sz, perm);
   int page_num = (page_num % 0x1000) ? ((sz >> 12) + 1) : (sz >> 12);
   //printk("page number = %d\n", page_num);
   for(int i = 0; i < page_num; i++) {
        uint64 curva = va + i * 0x1000;
        uint64 curpa = pa + i * 0x1000;

        //取出va + i * 0x1000的[38, 30]位作为vpn2
        uint64 vpn2 = ((va + i * 0x1000) & 0x0000007FC0000000) >> 30;
        //取出va + i * 0x1000的[29, 21]位作为vpn1
        uint64 vpn1 = ((va + i * 0x1000) & 0x000000003FE00000) >> 21;
        //取出va + i * 0x1000的[20, 12]位作为vpn0
        uint64 vpn0 = ((va + i * 0x1000) & 0x00000000001FF000) >> 12;

        //取出pa + i * 0x1000的[55, 30]位作为ppn2
        uint64 ppn2 = ((pa + i * 0x1000) & 0x00FFFFFFC0000000) >> 30;
        //取出pa + i * 0x1000的[29, 21]位作为ppn1
        uint64 ppn1 = ((pa + i * 0x1000) & 0x000000003FE00000) >> 21;
        //取出pa + i * 0x1000的[20, 12]位作为ppn0
        uint64 ppn0 = ((pa + i * 0x1000) & 0x00000000001FF000) >> 12;

        //printk("\nFirst Page\n");
        uint64 fpte = *(uint64*)((uint64)pgtbl + (vpn2 << 3));
        if(!(fpte & 0x1)) {
            uint64 content = kalloc() - PA2VA_OFFSET;
            fpte = (content & 0x00FFFFFFFFFFF000) >> 2;
            fpte = fpte | 0x1;
        }
        
        *(uint64*)((uint64)pgtbl + (vpn2 << 3)) = fpte;

        //printk("\nSecond Page\n");
        uint64 spa = ((fpte & 0x003FFFFFFFFFFFC00) << 2) + (vpn1 << 3) + PA2VA_OFFSET;
        uint64 spte = *(uint64*)(spa);
        if(!(spte & 0x1)) {
            uint64 content = kalloc() - PA2VA_OFFSET;
            spte = (content & 0x00FFFFFFFFFFF000) >> 2;
            spte = spte | 0x1;
        }
        
        *(uint64*)(spa) = spte;

        //printk("\nThird Page\n");
        uint64 tpa = ((spte & 0x003FFFFFFFFFFFC00) << 2) + (vpn0 << 3) + PA2VA_OFFSET;
        uint64 tpte = ((curpa & 0x00FFFFFFFFFFF000) >> 2) | 0x1 | (perm << 1);
        *(uint64*)(tpa) = tpte;
   }
}

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);
    //Debug
    // printk("setup_vm_final\n");
    // printk("_stext = %lx\n", (uint64)_stext);
    // printk("_srodata = %lx\n", (uint64)_srodata);
    // printk("_sdata = %lx\n", (uint64)_sdata);
    // printk("_sbss = %lx\n", (uint64)_sbss);
    // printk("_ekernel = %lx\n", (uint64)_ekernel);
    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping((uint64*)swapper_pg_dir, VM_START + OPENSBI_SIZE, PHY_START + OPENSBI_SIZE, (uint64)_srodata - (uint64)_stext, 5);
    //printk("va = %lx, pa = %lx, length = %lx\n", VM_START + OPENSBI_SIZE, PHY_START + OPENSBI_SIZE, (uint64)_srodata - (uint64)_stext);
    // mapping kernel rodata -|-|R|V
    create_mapping((uint64*)swapper_pg_dir, VM_START + OPENSBI_SIZE + (uint64)_srodata - (uint64)_stext, PHY_START + OPENSBI_SIZE + (uint64)_srodata - (uint64)_stext, (uint64)_sdata - (uint64)_srodata, 1);
    //printk("va = %lx, pa = %lx, length = %lx\n", VM_START + OPENSBI_SIZE + (uint64)_srodata - (uint64)_stext, PHY_START + OPENSBI_SIZE + (uint64)_srodata - (uint64)_stext, (uint64)_sdata - (uint64)_srodata);
    // mapping other memory -|W|R|V
    create_mapping((uint64*)swapper_pg_dir, VM_START + OPENSBI_SIZE + (uint64)_sdata - (uint64)_stext, PHY_START + OPENSBI_SIZE + (uint64)_sdata - (uint64)_stext, PHY_SIZE - OPENSBI_SIZE - (uint64)_sdata + (uint64)_stext, 3);
    // printk("va = %lx, pa = %lx, length = %lx\n", VM_START + OPENSBI_SIZE + (uint64)_sdata - (uint64)_stext, PHY_START + OPENSBI_SIZE + (uint64)_sdata - (uint64)_stext, PHY_SIZE - OPENSBI_SIZE - (uint64)_sdata + (uint64)_stext);
    // printk("map down!!!\n");
    
    // set satp with swapper_pg_dir
    uint64 val = (uint64)swapper_pg_dir - PA2VA_OFFSET;
    //printk("val = %lx\n", val);
    val = val >> 12;
    uint64 tmp = 1;
    tmp = tmp << 63;
    val = val | tmp;
    // printk("satp = %lx\n", val);
    
    csr_write(satp, val);
    // printk("set satp done\n");
    // flush TLB
    asm volatile("sfence.vma zero, zero");
    
    printk("setup_vm_final done\n");
    return;
}