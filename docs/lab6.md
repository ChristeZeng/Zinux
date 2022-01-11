# Lab 6: RV64 缺页异常处理以及 fork 机制

## 1. 实验目的
* 通过 **vm_area_struct** 数据结构实现对进程**多区域**虚拟内存的管理。
* 在 **Lab5** 实现用户态程序的基础上，添加缺页异常处理 **Page Fault Handler**。
* 为进程加入 **fork** 机制，能够支持通过 **fork** 创建新的用户态进程。

## 2. 实验环境 
* Docker in Lab0

## 3. 背景知识

### 3.1 vm_area_struct 介绍
在linux系统中，`vm_area_struct` 是虚拟内存管理的基本单元， `vm_area_struct` 保存了有关连续虚拟内存区域(简称vma)的信息。linux 具体某一进程的虚拟内存区域映射关系可以通过 [procfs【Link】](https://man7.org/linux/man-pages/man5/procfs.5.html) 读取 `/proc/pid/maps` 的内容来获取:

比如，如下一个常规的 `bash` 进程，假设它的进程号为 `7884` ，则通过输入如下命令，就可以查看该进程具体的虚拟地址内存映射情况(部分信息已省略)。

```shell
#cat /proc/7884/maps
556f22759000-556f22786000 r--p 00000000 08:05 16515165                   /usr/bin/bash
556f22786000-556f22837000 r-xp 0002d000 08:05 16515165                   /usr/bin/bash
556f22837000-556f2286e000 r--p 000de000 08:05 16515165                   /usr/bin/bash
556f2286e000-556f22872000 r--p 00114000 08:05 16515165                   /usr/bin/bash
556f22872000-556f2287b000 rw-p 00118000 08:05 16515165                   /usr/bin/bash
556f22fa5000-556f2312c000 rw-p 00000000 00:00 0                          [heap]
7fb9edb0f000-7fb9edb12000 r--p 00000000 08:05 16517264                   /usr/lib/x86_64-linux-gnu/libnss_files-2.31.so
7fb9edb12000-7fb9edb19000 r-xp 00003000 08:05 16517264                   /usr/lib/x86_64-linux-gnu/libnss_files-2.31.so                 
...
7ffee5cdc000-7ffee5cfd000 rw-p 00000000 00:00 0                          [stack]
7ffee5dce000-7ffee5dd1000 r--p 00000000 00:00 0                          [vvar]
7ffee5dd1000-7ffee5dd2000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```

从中我们可以读取如下一些有关该进程内虚拟内存映射的关键信息：

* `vm_start` :  (第1列) 指的是该段虚拟内存区域的开始地址
* `vm_end` :  (第2列) 指的是该段虚拟内存区域的结束地址
* `vm_flags` :  (第3列) 该 `vm_area` 的一组权限(rwx)标志， `vm_flags` 的具体取值定义可参考linux源代码的 [linux/mm.h](https://elixir.bootlin.com/linux/v5.14/source/include/linux/mm.h#L265)
* `vm_pgoff` :  (第4列) 虚拟内存映射区域在文件内的偏移量
* `vm_file` :  (第5/6/7列)分别表示：映射文件所属设备号/以及指向关联文件结构的指针(如果有的话，一般为文件系统的inode)/以及文件名

其它保存在 `vm_area_struct` 中的信息还有：

* `vm_ops` :  该`vm_area`中的一组工作函数
* `vm_next/vm_prev`: 同一进程的所有虚拟内存区域由**链表结构**链接起来，这是分别指向前后两个 `vm_area_struct` 结构体的指针

### 3.2 缺页异常 Page Fault
缺页异常是一种正在运行的程序访问当前未由内存管理单元（ MMU ）映射到虚拟内存的页面时，由计算机硬件引发的异常类型。访问未被映射的页或访问权限不足，都会导致该类异常的发生。处理缺页异常通常是操作系统内核的一部分。当处理缺页异常时，操作系统将尝试使所需页面在物理内存中的位置变得可访问（建立新的映射关系到虚拟内存）。而如果在非法访问内存的情况下，即发现触发 `Page Fault` 的虚拟内存地址（ Bad Address ）不在当前进程 `vm_area_struct` 链表中所定义的允许访问的虚拟内存地址范围内，或访问位置的权限条件不满足时，缺页异常处理将终止该程序的继续运行。 

#### 3.2.1 RISC-V Page Faults
RISC-V 异常处理：当系统运行发生异常时，可即时地通过解析csr scause寄存器的值，识别如下三种不同的Page Fault。

**SCAUSE** 寄存器指示发生异常的种类：

| Interrupt | Exception Code | Description |
| --- | --- | --- |
| 0 | 12 | Instruction Page Fault |
| 0 | 13 | Load Page Fault |
| 0 | 15 | Store/AMO Page Fault |

#### 3.2.2 常规处理 **Page Fault** 的方式介绍
处理缺页异常时所需的信息如下：

* 触发 **Page Fault** 时访问的虚拟内存地址 VA。当触发 page fault 时，`stval` 寄存器被被硬件自动设置为该出错的VA地址
* 导致 **Page Fault** 的类型：
    * Exception Code = 12: page fault caused by an instruction fetch 
    * Exception Code = 13: page fault caused by a read  
    * Exception Code = 15: page fault caused by a write 
* 发生 **Page Fault** 时的指令执行位置，保存在 `sepc` 中
* 当前进程合法的 **VMA** 映射关系，保存在`vm_area_struct`链表中
 
### 3.3 `fork` 系统调用
* `fork()`通过复制当前进程创建一个新的进程，新进程称为子进程，而原进程称为父进程。
* 子进程和父进程在不同的内存空间上运行。
* 父进程`fork`成功时`返回：子进程的pid`，子进程`返回：0`。`fork`失败则父进程`返回：-1`。
* 创建的子进程需要拷贝父进程 `task_struct`、`pgd`、`mm_struct` 以及父进程的 `user stack` 等信息。
* Linux 中使用了 `copy-on-write` 机制，`fork` 创建的子进程首先与父进程共享物理内存空间，直到父子进程有修改内存的操作发生时再为子进程分配物理内存。

## 4 实验步骤
### 4.1 准备工作
* 此次实验基于 lab5 同学所实现的代码进行。
* 从 repo 同步以下文件夹: user 并按照以下步骤将这些文件正确放置。
```
.
└── user
    ├── Makefile
    ├── getpid.c
    ├── link.lds
    ├── printf.c
    ├── start.S
    ├── stddef.h
    ├── stdio.h
    ├── syscall.h
    └── uapp.S
```
* 在 `user/getpid.c` 中我们设置了三个 `main` 函数。在实现了 `Page Fault` 之后第一个 `main` 函数可以成功运行。在实现了 `fork` 之后其余两个 `main` 函数可以成功运行。
* 修改 `task_init` 函数中修改为仅初始化一个进程，之后其余的进程均通过 `fork` 创建。

### 4.2 实现虚拟内存管理功能：
* 修改 `proc.h` 如下
```c
/* vm_area_struct vm_flags */
#define VM_READ		0x00000001
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004

struct vm_area_struct {
	struct mm_struct *vm_mm;    /* The mm_struct we belong to. */
	uint64_t vm_start;          /* Our start address within vm_mm. */
	uint64_t vm_end;            /* The first byte after our end address 
                                    within vm_mm. */

	/* linked list of VM areas per task, sorted by address */
	struct vm_area_struct *vm_next, *vm_prev;

	uint64_t vm_flags;      /* Flags as listed above. */
};

struct mm_struct {
	struct vm_area_struct *mmap;       /* list of VMAs */
};

struct task_struct {
    struct thread_info* thread_info;
    uint64_t state;
    uint64_t counter;
    uint64_t priority;
    uint64_t pid;

    struct thread_struct thread;

    pagetable_t pgd;

    struct mm_struct *mm;
};
```
* 每一个 vm_area_struct 都对应于进程地址空间的唯一区间。
* 为了支持 `Demand Paging`（见 4.3），我们需要支持对 vm_area_struct 的添加，查找。
```c
/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64_t addr) {

}

/*
 * @mm     : current thread's mm_struct
 * @addr   : the suggested va to map
 * @length : memory size to map
 * @prot   : protection
 *
 * @return : start va
*/
uint64_t do_mmap(struct mm_struct *mm, uint64_t addr, uint64_t length, int prot) {
    /*
    需要注意的是 do_mmap 中的 addr 可能无法被满足
    比如 addr 已经被占用, 因此需要判断，
    如果不合法，需要重新找到一个合适的位置进行映射。 
    */
}
```

### 4.3 Page Fault Handler
* `Demand Paging`
    * 在映射页面时，我们不真正对页表进行修改，只在 `mm->mmap` 链表上添加一个 `vma` 记录。
    * 当我们真正访问这个页面时，我们需要根据缺页的地址，找到其所在的 `vma`，根据 `vma` 中的信息对页表进行映射。
* 修改 `task_init` 函数代码，更改为 `Demand Paging`
    * 删除之前实验中对 `U-MODE` 代码，栈进行映射的代码
    * 调用 `do_mmap` 函数，建立用户进程的虚拟地址空间信息，包括两个区域:
        * 代码区域, 该区域从虚拟地址 `USER_START` 开始，大小为 `uapp_end - uapp_start`， 权限为 `VM_READ | VM_WRITE | VM_EXEC`
        * 用户栈，范围为 `[USER_END - PGSIZE, USER_END)` ，权限为 `VM_READ | VM_WRITE`
* 在完成上述修改之后，如果运行代码我们可以截获 一个 page fault. 如下图 （注意：由于试例代码以及正确处理 page fault， 所以我们可以看到一系列的page fault ）
```bash 

// Instruction Page Fault
scause = 0x000000000000000c, sepc = 0x0000000000000000, stval = 0x0000000000000000 

// Store/AMO Page Fault: sepc 是 code address， stval 是 写入的地址 (位于 user stack 内)。
scause = 0x000000000000000f, sepc = 0x0000000000000070, stval = 0x0000003ffffffff8 

************************** uapp asm **************************
 .....

 Disassembly of section .text.main:

 000000000000006c <main>:
   6c:   fe010113                addi    sp,sp,-32
   70:   00113c23                sd      ra,24(sp) <- Page Fault
   74:   00813823                sd      s0,16(sp)
   78:   02010413                addi    s0,sp,32
   7c:   fbdff0ef                jal     ra,38 <fork>
   80:   00050793                mv      a5,a0
   84:   fef42223                sw      a5,-28(s0)
   88:   fe442783                lw      a5,-28(s0)

......
************************** uapp asm **************************
```
* 实现 Page Fault 的检测与处理
    * 修改`trap.c`, 添加捕获 Page Fault 的逻辑
    * 当捕获了 `Page Fault` 之后，需要实现缺页异常的处理函数  `do_page_fault`
```c
void do_page_fault(struct pt_regs *regs) {
    /*
    1. 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
    2. 通过 sepc 获得当前的 Page Fault 类型
    3. 通过 find_vm() 找到匹配的 vm_area
    4. 通过 vm_area 的 vm_flags 对当前的 Page Fault 类型进行检查
        4.1 Instruction Page Fault      -> VM_EXEC
        4.2 Load Page Fault             -> VM_READ
        4.3 Store Page Fault            -> VM_WRITE
    5. 最后调用 create_mapping 对页表进行映射
    */
}
```

### 4.4 实现 fork()
* 修改 `task_struct` 增加结构成员 `trapframe`， 如下：
```c
struct task_struct {
    struct thread_info* thread_info;
    uint64_t state;
    uint64_t counter;
    uint64_t priority;
    uint64_t pid;

    struct thread_struct thread;

    pagetable_t pgd;

    struct mm_struct *mm;
    
    struct pt_regs *trapframe;
};
```
`trapframe` 成员用于保存异常上下文，当我们 `fork` 出来一个子进程时候，我们将父进程的上下文环境复制到子进程的 `trapframe` 中。当子进程被调度时候，我们可以通过 `trapframe` 来恢复上下文环境。
* fork() 所调用的 syscall 为 SYS_CLONE，系统调用号为 220
```c
#define SYS_CLONE 220
```
* 实现 `clone`， 为了简单起见 `clone` 只接受一个参数 `pt_regs *`
```c
void forkret() {
    ret_from_fork(current->trapframe);
}

uint64_t do_fork(struct pt_regs *regs) {
    /*
    1. 参考 task_init 创建一个新的 子进程, 将 thread.ra 设置为 forkret
    2. 为子进程申请 user stack, 并将父进程的 user stack 数据复制到其
       中. 同时将子进程的 user stack 的地址保存在 thread_info->user_sp 中
    3. 在 task_init 中我们进程的 user-mode sp 可以直接设置为 
       USER_END, 但是这里需要设置为当前进程(父进程)的 user-mode sp
    4. 将父进程上下文环境保存到子进程的 trapframe 中
    5. 将子进程的 trapframe->a0 修改为 0
    6. 复制父进程的 mm_struct 以及 vm_area_struct
    7. 创建子进程的页表 这里只做内核的映射,即 swapper_pg_dir
    8. 返回子进程的 pid
    */
}

uint64_t clone(struct pt_regs *regs) {
    return do_fork(regs);
}
```
* 参考 `_trap` 中的恢复逻辑 在 `entry.S` 中实现 `ret_from_fork`, 函数原型如下：
    * 注意恢复寄存器的顺序
    * `_trap` 中是从 `stack` 上恢复，这里从 `trapframe` 中恢复
```c
void ret_from_fork(struct pt_regs *trapframe);
```
* 修改 `Page Fault` 处理:
    * 在之前的 `Page Fault` 处理中，我们对用户栈 `Page Fault` 处理方法是自由分配一页作为用户栈并映射到`[USER_END - PAGE_SIZE, USER_END)`的虚拟地址。但由 `fork` 创建的进程，它的用户栈已经拷贝完毕，因此 `Page Fault` 处理时直接为该页建立映射即可 ( 通过  `thread_info->user_sp` 来进行判断)。

### 4.5 编译及测试
- 输出示例
    ```bash
    OpenSBI v0.9
         ____                    _____ ____ _____
        / __ \                  / ____|  _ \_   _|
        | |  | |_ __   ___ _ __ | (___ | |_) || |
        | |  | | '_ \ / _ \ '_ \ \___ \|  _ < | |
        | |__| | |_) |  __/ | | |____) | |_) || |_
        \____/| .__/ \___|_| |_|_____/|____/_____|
            | |
            |_|

        ...

        Boot HART MIDELEG         : 0x0000000000000222
        Boot HART MEDELEG         : 0x000000000000b109

        ...mm_init done!
        ...proc_init done!
        [S-MODE] Hello RISC-V

        scause = 0x000000000000000c, sepc = 0x0000000000000000, stval = 0x0000000000000000
        scause = 0x000000000000000f, sepc = 0x0000000000000070, stval = 0x0000003ffffffff8
        [PID = 1] fork [PID = 2]
        [PID = 1] fork [PID = 3]
        [PID = 1] is running!

        scause = 0x000000000000000c, sepc = 0x0000000000000050, stval = 0x0000000000000050
        scause = 0x000000000000000f, sepc = 0x0000000000000054, stval = 0x0000003fffffffc8
        [PID = 3] is running!

        scause = 0x000000000000000c, sepc = 0x0000000000000050, stval = 0x0000000000000050
        scause = 0x000000000000000f, sepc = 0x0000000000000054, stval = 0x0000003fffffffc8
        [PID = 2] fork [PID = 4]
        [PID = 2] is running!

        [PID = 1] is running!

        scause = 0x000000000000000c, sepc = 0x0000000000000050, stval = 0x0000000000000050
        scause = 0x000000000000000f, sepc = 0x0000000000000054, stval = 0x0000003fffffffc8
        [PID = 4] is running!

        [PID = 3] is running!

        [PID = 2] is running!
        ...

    ```

## 作业提交
同学需要提交实验报告以及整个工程代码。在提交前请使用 `make clean` 清除所有构建产物。
