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
   
}

// arch/riscv/kernel/vm.c 

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

uint64 read_piece(uint64 n, int left, int right) {
	uint64 res;
	res = (n << (63 - left)) >> (63 + right - left);
	return res;
}

uint64 write_piece(uint64 n, uint64 content, int left, int right) {
	int i;
	// clear
	for (i = right; i <= left; i++) {
		n &= ~(1 << i);
	}
	// set
	n |= content << right;
	return n;
}

uint64 read_mem(uint64 *p) {
	return (uint64)(*p);
}

void write_mem(uint64 *p, uint64 val) {
	*p = val;
	return;
}

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
   printk("\ncreate_mapping: pgtbl: %lx, va: %lx, pa: %lx, sz: %lx, perm: %d\n", pgtbl, va, pa, sz, perm);
   int page_num = (page_num % 0x1000) ? ((sz >> 12) + 1) : (sz >> 12);
   printk("page number = %d\n", page_num);
   for(int i = 0; i < page_num; i++) {
        // //取出va + i * 0x1000的[38, 30]位作为vpn2
        // uint64 vpn2 = ((va + i * 0x1000) & 0x0000007FC0000000) >> 30;
        // //取出va + i * 0x1000的[29, 21]位作为vpn1
        // uint64 vpn1 = ((va + i * 0x1000) & 0x000000003FE00000) >> 21;
        // //取出va + i * 0x1000的[20, 12]位作为vpn0
        // uint64 vpn0 = ((va + i * 0x1000) & 0x00000000001FF000) >> 12;

        // //取出pa + i * 0x1000的[55, 30]位作为ppn2
        // uint64 ppn2 = ((pa + i * 0x1000) & 0x00FFFFFFC0000000) >> 30;
        // //取出pa + i * 0x1000的[29, 21]位作为ppn1
        // uint64 ppn1 = ((pa + i * 0x1000) & 0x000000003FE00000) >> 21;
        // //取出pa + i * 0x1000的[20, 12]位作为ppn0
        // uint64 ppn0 = ((pa + i * 0x1000) & 0x00000000001FF000) >> 12;

        // uint64 first_ptl_addr = (uint64)pgtbl + (vpn2 << 3);
        // uint64 first_pte = *(uint64 *)first_ptl_addr;
        // if(!(first_pte & 0x1)) {
        //     //如果页表项不存在，则创建页表项
        //     uint64 content = kalloc();
        //     first_pte = (content & 0xFFFFFFFFFFFFF000) | 0x1;
            
        // }
        uint64 curva = va + i * 0x1000;
        uint64 curpa = pa + i * 0x1000;
        uint64 vpn2, vpn1, vpn0, ppn2, ppn1, ppn0;
        vpn2 = read_piece(curva, 38, 30);
        vpn1 = read_piece(curva, 29, 21);
        vpn0 = read_piece(curva, 20, 12);
        ppn2 = read_piece(curpa, 55, 30);
        ppn1 = read_piece(curpa, 29, 21);
        ppn0 = read_piece(curpa, 20, 12);
        printk("\n %dth map, curva = %lx, curpa = %lx\n", i + 1, curva, curpa);
        printk("vpn2 = %lx, vpn1 = %lx, vpn0 = %lx\n", vpn2, vpn1, vpn0);
        printk("ppn2 = %lx, ppn1 = %lx, ppn0 = %lx\n", ppn2, ppn1, ppn0);
        
        printk("\nFirst Page\n");
        uint64 first_page_addr = (uint64)pgtbl + (vpn2 << 3);
        
        printk("first_page_addr = %lx\n", first_page_addr);
        
        uint64 first_page_entry;
        first_page_entry = read_mem((uint64*)first_page_addr);
        
        printk("first_page_entry = %lx\n", first_page_entry);

        if(!(first_page_entry & 0x1)) {
            printk("first_page_entry is not exist\n");
            uint64 content = kalloc() - PA2VA_OFFSET;
            printk("content = %lx\n", content);
            printk("content = %lx\n", content >> 12);
            first_page_entry = write_piece(first_page_entry, content >> 12, 53, 10);
            first_page_entry = write_piece(first_page_entry, 0x1, 0, 0);
            printk("first_page_entry_after_kalloc = %lx\n", first_page_entry);
        }
        
        write_mem((uint64*)first_page_addr, first_page_entry);
        printk("first_page_entry_after_write = %lx\n", read_mem((uint64*)first_page_addr));

        printk("\nSecond Page\n");
        uint64 second_page_addr = (read_piece(read_mem((uint64*)first_page_addr), 53, 10) << 12) + (vpn1 << 3);
        printk("second_page_addr = %lx\n", second_page_addr);
        uint64 second_page_entry;
        second_page_entry = read_mem((uint64*)second_page_addr);
        //printk("second_page_entry = %lx\n", second_page_entry);
        if(!(second_page_entry & 0x1)) {
            //printk("second_page_entry is not exist\n");
            uint64 content = kalloc() - PA2VA_OFFSET;
            //printk("content = %lx\n", content);
            second_page_entry = write_piece(second_page_entry, content >> 12, 53, 10);
            second_page_entry = write_piece(second_page_entry, 0x1, 0, 0);
            //printk("second_page_entry_after_kalloc = %lx\n", second_page_entry);
        }
        
        write_mem((uint64*)second_page_addr, second_page_entry);
        //printk("second_page_entry_after_write = %lx\n", read_mem((uint64*)second_page_addr));

        //printk("\nThird Page\n");
        uint64 third_page_addr = (read_piece(read_mem((uint64*)second_page_addr), 53, 10) << 12) + 0xff00000000000000 + (vpn0 << 3);
        //printk("third_page_addr = %lx\n", third_page_addr);

        uint64 third_page_entry = 0x0;
        third_page_entry = write_piece(third_page_entry, curpa >> 12, 53, 10);
        //printk("third_page_entry = %lx\n", third_page_entry);

        uint64 perm_with_valid = (perm << 1) | 0b1;
        third_page_entry = write_piece(third_page_entry, perm_with_valid, 3, 0);
        
        write_mem((uint64*)third_page_addr, third_page_entry);
        //printk("third_page_entry_after_write = %lx\n", third_page_entry);
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
    printk("va = %lx, pa = %lx, length = %lx\n", VM_START + OPENSBI_SIZE, PHY_START + OPENSBI_SIZE, (uint64)_srodata - (uint64)_stext);
    // mapping kernel rodata -|-|R|V
    create_mapping((uint64*)swapper_pg_dir, VM_START + OPENSBI_SIZE + (uint64)_srodata - (uint64)_stext, PHY_START + OPENSBI_SIZE + (uint64)_srodata - (uint64)_stext, (uint64)_sdata - (uint64)_srodata, 1);
    printk("va = %lx, pa = %lx, length = %lx\n", VM_START + OPENSBI_SIZE + (uint64)_srodata - (uint64)_stext, PHY_START + OPENSBI_SIZE + (uint64)_srodata - (uint64)_stext, (uint64)_sdata - (uint64)_srodata);
    // mapping other memory -|W|R|V
    create_mapping((uint64*)swapper_pg_dir, VM_START + OPENSBI_SIZE + (uint64)_sdata - (uint64)_stext, PHY_START + OPENSBI_SIZE + (uint64)_sdata - (uint64)_stext, PHY_SIZE - OPENSBI_SIZE - (uint64)_sdata + (uint64)_stext, 3);
    printk("va = %lx, pa = %lx, length = %lx\n", VM_START + OPENSBI_SIZE + (uint64)_sdata - (uint64)_stext, PHY_START + OPENSBI_SIZE + (uint64)_sdata - (uint64)_stext, PHY_SIZE - OPENSBI_SIZE - (uint64)_sdata + (uint64)_stext);
    printk("map down!!!\n");
    
    // set satp with swapper_pg_dir
    uint64 val = (uint64)swapper_pg_dir - PA2VA_OFFSET;
    printk("val = %lx\n", val);
    val = val >> 12;
    uint64 tmp = 1;
    tmp = tmp << 63;
    val = val | tmp;
    printk("satp = %lx\n", val);
    
    csr_write(satp, val);
    printk("set satp done\n");

    //vmtest();
    // flush TLB
    printk("flush TLB\n");
    asm volatile("sfence.vma zero, zero");
    printk("setup_vm_final done\n");
    return;
}

void vmtest() {
    uint64 read_val = 0x0;
	uint64 write_val = 0xabcdef1234567890;
	uint64 *other2_addr = (uint64*)0x00ffffe000200000;
	uint64 *other_addr = (uint64*)0xffffffe000200000;

    printk("before read_mem: ");
	printk("%lx", read_val);
	read_val = read_mem(other_addr);
	printk(", after read_mem: ");
	printk("%lx\n", read_val);
	printk("r is permitted!\n");
	printk("before write_mem: ");
	printk("%lx", read_val);
	write_mem(other_addr, write_val);
	read_val = read_mem(other2_addr);
	printk(", after write_mem: ");
	printk("%lx", read_val);
	printk("\nx is permitted!\n");
	// asm volatile("li t0, 0xffffffe000003100");
	// asm volatile("jalr t0");
	// printk("x is permitted!\n");
}
