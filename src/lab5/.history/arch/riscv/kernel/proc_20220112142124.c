#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
extern void __dummy();
#define _PROC_MAX_N 1917046782

struct task_struct* idle;
struct task_struct* current;
struct task_struct* task[NR_TASKS];

extern char uapp_start[];
extern char uapp_end[];

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
        // printk("%lx\n", temp);
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

// This is only needed to show the print information
// Since a new task with counter 1, which will be reduced to 0
// will be the same to last counter
// making the information not printed
unsigned int switch_bit;
void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter || switch_bit) {
            // Clean the switch bit to make it impossible to print twice after a switch
            switch_bit = 0;
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            // printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var); 
            printk("[PID = %d] is running. Thread space begins at = %lx\n", current->pid, current); 
        }
    }
}

extern void __switch_to(struct task_struct* prev, struct task_struct* next);


void switch_to(struct task_struct* next) {
    switch_bit = 1;
    if (current != next) {
        struct task_struct* temp = current;
        current = next;
        next->counter--;
        __switch_to(temp, next);
    } else {
        // Only need to do counter--
        // But not real switch
        next->counter--;
    }
}

void do_timer(void) {
    if (current == idle || current->counter == 0) {
        // If current task's time slice have been run out 
        // Do schedule
        schedule();
    } else {
        // Else, simply reduce time slice held
        current->counter--;
    }
}

void schedule(void) {
    // Search for the task with least time slice
#ifdef SJF
#define _SJF
    int least_counter = _PROC_MAX_N;
    int least_task = 0;
    for (int i = 1; i < NR_TASKS; i++) {
        if (task[i]->counter < least_counter && task[i]->counter > 0) {
            least_counter = task[i]->counter;
            least_task = i;
        }
    }

    // If all the time slice have been run out
    // Reassign random values
    if (least_counter == _PROC_MAX_N) {
        for (int i = 1; i < NR_TASKS; i++) {
            task[i]->counter = rand();
            printk("SET: pid: %d, priority: %d, counter: %d\n", task[i]->pid, task[i]->priority, task[i]->counter);
        }
        for (int i = 1; i < NR_TASKS; i++) {
            if (task[i]->counter < least_counter && task[i]->counter > 0) {
                least_counter = task[i]->counter;
                least_task = i;
            }
        }
    }
    // Switch to the task found
    printk("SWITCH: pid: %d, priority: %d, counter: %d\n", task[least_task]->pid, task[least_task]->priority, task[least_task]->counter);
    switch_to(task[least_task]);
#endif

#ifdef PRIORITY
#define _PRIORITY
    int greatest_counter = 0;
    int greatest_task = 0;
    for (int i = 1; i < NR_TASKS; i++) {
        if (task[i]->counter > greatest_counter) {
            greatest_counter = task[i]->counter;
            greatest_task = i;
        }
    }
    if (greatest_task == 0) {
        for (int i = 1; i < NR_TASKS; i++) {
            task[i]->counter = (task[i]->counter >> 1) + task[i]->priority;
        }
        for (int i = 1; i < NR_TASKS; i++) {
            if (task[i]->counter > greatest_counter) {
                greatest_counter = task[i]->counter;
                greatest_task = i;
            }
        }
    }
    printk("SWITCH: pid: %d, priority: %d, counter: %d\n", task[greatest_task]->pid, task[greatest_task]->priority, task[greatest_task]->counter);
    switch_to(task[greatest_task]);
#endif 
}