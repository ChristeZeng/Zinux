//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"

extern void __dummy();

extern void __switch_to(struct task_struct* prev, struct task_struct* next);

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此

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
    for(int i = 1; i < NR_TASKS; i++){
        task[i] = (struct task_struct*)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE; //?
    }

    printk("...proc_init done!\n");
}

void dummy() {
    printk("dummy\n");
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}

void switch_to(struct task_struct* next) {
    printk("switch_to\n");
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
    printk("do_timer\n");
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
    printk("\n")
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
            if(task[i]->state == TASK_RUNNING && task[i]->priority > max_pri && task[i]->counter > 0) {
                max_pri = task[i]->priority;
                index_to_switch = i;
                isallzero = 0;
            }
        }

        if(isallzero) {
            for(int i = 1; i <= NR_TASKS - 1; i++) {
                printk("\n");
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