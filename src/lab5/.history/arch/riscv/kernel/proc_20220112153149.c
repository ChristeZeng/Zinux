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

typedef uint64 bool;

static unsigned long construct_entry(unsigned long pm_addr, bool d, bool a, bool g, bool u, bool x, bool w, bool r, bool v) {
    unsigned long ret = pm_addr >> 12 << 10;
    ret |= d << 7;
    ret |= a << 6;
    ret |= g << 5;
    ret |= u << 4;
    ret |= x << 3;
    ret |= w << 2;
    ret |= r << 1;
    ret |= v << 0;
    return ret;
}


static void create_user_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, bool x, bool w, bool r) {
    
    for (unsigned long i = va; i < va + sz; i += 0x1000) {
        if (!is_valid_entry(pgtbl[vm_addr_to_index_fisrt(i)])) {
            unsigned long *vm_addr = (unsigned long*)kalloc();
            unsigned long pm_addr = (unsigned long)vm_addr - PA2VA_OFFSET;
            pgtbl[vm_addr_to_index_fisrt(i)] = construct_entry(pm_addr, 0, 0, 0, 1, 0, 0, 0, 1);
        }
        unsigned long first_entry = pgtbl[vm_addr_to_index_fisrt(i)];
        unsigned long pm_addr_second_table = first_entry << 10 >> 20 << 12;
        unsigned long* vm_addr_second_table = (unsigned long*)(pm_addr_second_table + PA2VA_OFFSET);

        if (!is_valid_entry(vm_addr_second_table[vm_addr_to_index_second(i)])) {
            unsigned long *vm_addr = (unsigned long*)kalloc();
            unsigned long pm_addr = (unsigned long)vm_addr - PA2VA_OFFSET;
            vm_addr_second_table[vm_addr_to_index_second(i)] = construct_entry(pm_addr, 0, 0, 0, 1, 0, 0, 0, 1);
        }
        unsigned long second_entry = vm_addr_second_table[vm_addr_to_index_second(i)];
        unsigned long pm_addr_third_table = second_entry << 10 >> 20 << 12;
        unsigned long* vm_addr_third_table = (unsigned long*)(pm_addr_third_table + PA2VA_OFFSET);

        if (!is_valid_entry(vm_addr_third_table[vm_addr_to_index_third(i)])) {
            unsigned long pm_addr = pa + (i - va);
            vm_addr_third_table[vm_addr_to_index_third(i)] = construct_entry(pm_addr, 0, 0, 0, 1, x, w, r, 1);
        }
        // if (!w)
        // show_page(vm_addr_third_table);
    }
}

extern unsigned long swapper_pg_dir[];

void task_init() {
    idle = (struct task_struct*)kalloc();
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    task[0] = idle;
    current = idle;

    struct task_struct* temp;
    for (int i = 1; i < NR_TASKS; i++) {
        temp = (struct task_struct*)kalloc();
        
        temp->state = TASK_RUNNING;
        temp->counter = 0;
        temp->priority = rand();
        temp->pid = i;
        // Assign the return address, making every task shceduled for the first time will 
        // return to the dummy() funciton
        // and print some useful infomation
        temp->thread.ra = (uint64)__dummy;
        // Set the stack top
        // This is the kernel space stack top
        temp->thread.sp = (unsigned long long)temp + 0x1000;
        task[i] = temp;

        temp->pgd = (uint64)kalloc();
        printk("pdg address %lx\n", temp->pgd);
        // uint64 temp_entry = swapper_pg_dir[384];
        // printk("temp_entry %lx\n", temp_entry);
        // temp->pgd[384] = 0x0000000021fffc01;
        // temp->pgd[384] = swapper_pg_dir[384];
        for (int j = 0; j < 512; j++) {
            temp->pgd[j] = swapper_pg_dir[j];
        }
        // printk("done, %lx\n", temp->pgd[384]);

        // User space stack
        uint64 u_mode_stack = (uint64)kalloc();
        uint64 pa_u_mode_stack = u_mode_stack - PA2VA_OFFSET;

        // Do map
        // printk("uapp_start: %lx\nuapp_start\n", uapp_start, uapp_end);
        uint64 pa_uapp_start = uapp_start - PA2VA_OFFSET;
        uint64 uapp_size = uapp_end - uapp_start;
        uint64 va_uapp_start = 0;
        // Code part
        create_user_mapping(temp->pgd, va_uapp_start, pa_uapp_start, uapp_size, 1, 1, 1);
        // Stack
        create_user_mapping(temp->pgd, USER_END - 0x1000, pa_u_mode_stack, 0x1000, 1, 1, 1);

        //CSRs
        temp->thread.sepc = USER_START;
        temp->thread.sstatus = 0x40020;
        temp->thread.sscratch = USER_END;

        // thread_info
        temp->thread_info = (uint64)kalloc();
        temp->thread_info->kernel_sp = temp->thread.sp;
        temp->thread_info->user_sp = USER_END;

        printk("INITIAL: pid: %d, priority: %d, counter: %d, kernel_sp: %lx, user_sp: %lx\n", temp->pid, temp->priority, temp->counter, temp->thread_info->kernel_sp, temp->thread_info->user_sp);
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