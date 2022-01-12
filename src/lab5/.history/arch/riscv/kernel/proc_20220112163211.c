//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"

extern char uapp_start[];
extern char uapp_end[];

extern void __dummy();

extern void __switch_to(struct task_struct* prev, struct task_struct* next);

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此

extern unsigned long swapper_pg_dir[];

void create_user_mapping2(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm){
    uint64* addr2;
    uint64* addr3;
    printk("%ld", sz);
    int page_num = (sz % 0x1000) ? ((sz >> 12) + 1) : (sz >> 12);
    printk("page_num: %d\n", page_num);
    for(int i = 0; i < page_num; i++){
        printk("%ld\n", i);
        uint64 PPN1 = ((va+i * PGSIZE)>>30)&0x1ff;
        uint64 PPN2 = ((va+i * PGSIZE)>>21)&0x1ff;
        uint64 PPN3 = ((va+i * PGSIZE)>>12)&0x1ff;
        if(!(pgtbl[PPN1]&0x1)) {
            addr2 = (uint64*)kalloc();
            addr2[PPN2] = 0;
            pgtbl[PPN1] += 0x1;
        } else {
            addr2 = (uint64)((uint64)((pgtbl[PPN1]>>10)<<12) + (uint64)PA2VA_OFFSET);
        }
        pgtbl[PPN1] = (((uint64)((uint64)addr2-PA2VA_OFFSET) >>12)<<10) | 0x11;
        if(!(addr2[PPN2]&0x1)){ 
            addr3 = (uint64*)kalloc();
            addr2[PPN2]+=0x1;
        } else {
            addr3 = ((addr2[PPN2]>>10)<<12) + PA2VA_OFFSET;
        }
        addr2[PPN2] = (((uint64)((uint64)addr3-PA2VA_OFFSET) >>12)<<10) | 0x11;
        addr3[PPN3] = (((pa+i * PGSIZE)>>12)<<10) | perm | 0x10;
    }
}

/* 创建多级页表映射关系 */
void create_mapping_user(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
   //printk("\ncreate_mapping: pgtbl: %lx, va: %lx, pa: %lx, sz: %lx, perm: %d\n", pgtbl, va, pa, sz, perm);
   int page_num = (sz % 0x1000) ? ((sz >> 12) + 1) : (sz >> 12);
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
            fpte = fpte | 0x11;
            printk("\nFirst Page: fpte: %lx\n", fpte);
        }
        
        *(uint64*)((uint64)pgtbl + (vpn2 << 3)) = fpte;

        //printk("\nSecond Page\n");
        uint64 spa = ((fpte & 0x003FFFFFFFFFFFC00) << 2) + (vpn1 << 3) + PA2VA_OFFSET;
        uint64 spte = *(uint64*)(spa);
        if(!(spte & 0x1)) {
            uint64 content = kalloc() - PA2VA_OFFSET;
            spte = (content & 0x00FFFFFFFFFFF000) >> 2;
            spte = spte | 0x11;
            printk("\nSecond Page: spte: %lx\n", spte);
        }
        
        *(uint64*)(spa) = spte;

        //printk("\nThird Page\n");
        uint64 tpa = ((spte & 0x003FFFFFFFFFFFC00) << 2) + (vpn0 << 3) + PA2VA_OFFSET;
        uint64 tpte = ((curpa & 0x00FFFFFFFFFFF000) >> 2) | 0x10 | perm;
        *(uint64*)(tpa) = tpte;
        printk("\nThird Page: tpte: %lx\n", tpte);
   }
}

void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle

    /* YOUR CODE HERE */
    idle = (struct task_struct*)kalloc();
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址， `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    // struct task_struct* temp;
    for(int i = 1; i < NR_TASKS; i++){
        task[i] = (struct task_struct*)kalloc();
        //printk("task[%d] = %lx\n", i, task[i]);
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE; 

        task[i]->thread.sepc = USER_START;
        task[i]->thread.sstatus = 0x40020;
        task[i]->thread.sscratch = USER_END;

        task[i]->pgd = (uint64)kalloc();
        //printk("task[%d]->pgd = %lx\n", i, task[i]->pgd);

        for(int j = 0; j < 512; j++){
            task[i]->pgd[j] = swapper_pg_dir[j];
        }
        //权限设置有冲突
        // create_user_mapping2(task[i]->pgd, USER_START, (uint64)uapp_start - PA2VA_OFFSET, (uint64)uapp_end - (uint64)uapp_start, 15);
        // create_user_mapping2(task[i]->pgd, USER_END - PGSIZE, (uint64)kalloc() - PA2VA_OFFSET, PGSIZE, 15);

        create_mapping_user(task[i]->pgd, USER_START, (uint64)uapp_start - PA2VA_OFFSET, (uint64)uapp_end - (uint64)uapp_start, 15);
        create_mapping_user(task[i]->pgd, USER_END - PGSIZE, (uint64)kalloc() - PA2VA_OFFSET, PGSIZE, 15);

        task[i]->thread_info = (uint64)kalloc();
        task[i]->thread_info->kernel_sp = task[i]->thread.sp;
        task[i]->thread_info->user_sp = USER_END;
        printk("Init: pid: %d, priority: %d, counter: %d, kernel_sp: %lx, user_sp: %lx\n", task[i]->pid, task[i]->priority, task[i]->counter, task[i]->thread_info->kernel_sp, task[i]->thread_info->user_sp);

    }

    printk("...proc_init done!\n");
}

void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. thread space begin at = %lx. \n", current->pid, (uint64)current);
        }
    }
}

void switch_to(struct task_struct* next) {
    //printk("switch_to\n");
    /* YOUR CODE HERE */
    if(current == next)
        return;
    else {
        struct task_struct* tmp = current;
        current = next;
        __switch_to(tmp, current); 
    }    
}

void do_timer(void) {
    /* 1. 如果当前线程是 idle 线程 直接进行调度 */
    /* 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减 1 
          若剩余时间任然大于0 则直接返回 否则进行调度 */

    /* YOUR CODE HERE */
    //printk("do_timer\n");
    if(current == idle)
        schedule();
    else {
        current->counter--;
        if(current->counter > 0)
            return;
        else
            schedule();
    }
}

void schedule(void) {
    /* YOUR CODE HERE */
#ifdef SJF
    int isallzero = 1;
    int index_to_switch = 0;
    int mini_couter = 100000;
    while(1) {
        isallzero = 1;
        index_to_switch = 0;
        mini_couter = 100000;

        for(int i = 1; i <= NR_TASKS - 1; i++) {
            if(task[i] && task[i]->state == TASK_RUNNING && task[i]->counter < mini_couter && task[i]->counter > 0) {
                mini_couter = task[i]->counter;
                index_to_switch = i;
                isallzero = 0;
            }
        }

        if(isallzero) {
            printk("\n");
            for(int i = 1; i <= NR_TASKS - 1; i++) {
                task[i]->counter = rand();
                printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
            }
        }
        else
            break;
    }
    printk("\n");
    printk("switch to [PID = %d COUNTER = %d]\n", task[index_to_switch]->pid, task[index_to_switch]->counter);
    switch_to(task[index_to_switch]);
#endif

#ifdef PRIORITY
    int isallzero = 1;
    int index_to_switch = 0;
    int max_pri = 0;
    while(1) {
        isallzero = 1;
        index_to_switch = 0;
        max_pri = 0;

        for(int i = 1; i <= NR_TASKS - 1; i++) {
            if(task[i] && task[i]->state == TASK_RUNNING && task[i]->priority > max_pri && task[i]->counter > 0) {
                max_pri = task[i]->priority;
                index_to_switch = i;
                isallzero = 0;
            }
        }

        if(isallzero) {
            printk("\n");
            for(int i = 1; i <= NR_TASKS - 1; i++) {
                task[i]->counter = task[i]->priority;
                printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i]->pid, task[i]->priority, task[i]->counter);
            }
        }
        else
            break;
    }
    printk("\n");
    printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n", task[index_to_switch]->pid, task[index_to_switch]->priority, task[index_to_switch]->counter);
    switch_to(task[index_to_switch]);
#endif
}