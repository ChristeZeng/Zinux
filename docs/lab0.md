# Lab 0: GDB + QEMU 调试 64 位 RISC-V LINUX

## 1 实验目的
- 了解容器的使用
- 使用交叉编译工具, 完成Linux内核代码编译
- 使用QEMU运行内核
- 熟悉GDB和QEMU联合调试

## 2 实验环境

- Docker
- 实验环境镜像[下载地址](https://pan.zju.edu.cn/share/ded28f6f578c955bfe94b79192)

## 3 实验基础知识介绍

### 3.1 Linux 使用基础

在Linux环境下，人们通常使用命令行接口来完成与计算机的交互。终端（Terminal）是用于处理该过程的一个应用程序，通过终端你可以运行各种程序以及在自己的计算机上处理文件。在类Unix的操作系统上，终端可以为你完成一切你所需要的操作。下面我们仅对实验中涉及的一些概念进行介绍，你可以通过下面的链接来对命令行的使用进行学习：

1. [The Missing Semester of Your CS Education](https://missing-semester-cn.github.io/2020/shell-tools)[&gt;&gt;Video&lt;&lt;](https://www.bilibili.com/video/BV1x7411H7wa?p=2)
2. [GNU/Linux Command-Line Tools Summary](https://tldp.org/LDP/GNU-Linux-Tools-Summary/html/index.html)
3. [Basics of UNIX](https://github.com/berkeley-scf/tutorial-unix-basics)

### 3.2 Docker 使用基础

#### Docker 基本介绍

Docker 是一种利用容器（container）来进行创建、部署和运行应用的工具。Docker把一个应用程序运行需要的二进制文件、运行需要的库以及其他依赖文件打包为一个包（package），然后通过该包创建容器并运行，由此被打包的应用便成功运行在了Docker容器中。之所以要把应用程序打包，并以容器的方式运行，主要是因为在生产开发环境中，常常会遇到应用程序和系统环境变量以及一些依赖的库文件不匹配，导致应用无法正常运行的问题。Docker带来的好处是只要我们将应用程序打包完成（组装成为Docker imgae），在任意安装了Docker的机器上，都可以通过运行容器的方式来运行该应用程序，因而将依赖、环境变量等带来的应用部署问题解决了。

如果想了解更多 Docker 的详情，请参考[官网](https://www.docker.com/)。

#### Docker 安装

请根据 [https://docs.docker.com/get-docker](https://docs.docker.com/get-docker) 自行在本机安装 Docker 环境。你可以从 [2 实验环境](#2) 中获得实验所需的环境，我们已经为你准备好了 RISC-V 工具链，以及 QEMU 模拟器，使用方法请参见 [4 实验步骤](#4)。

### 3.3 QEMU 使用基础

#### 什么是QEMU

QEMU 是一个功能强大的模拟器，可以在 x86 平台上执行不同架构下的程序。我们实验中采用 QEMU 来完成 RISC-V 架构的程序的模拟。

#### 如何使用 QEMU（常见参数介绍）

以以下命令为例，我们简单介绍 QEMU 的参数所代表的含义

```bash
$ qemu-system-riscv64 \
    -nographic \
    -machine virt \
    -kernel path/to/linux/arch/riscv/boot/Image \
    -device virtio-blk-device,drive=hd0 \
    -append "root=/dev/vda ro console=ttyS0" \
    -bios default \
    -drive file=rootfs.img,format=raw,id=hd0 \
    -S -s
```

- `-nographic`: 不使用图形窗口，使用命令行
- `-machine`: 指定要emulate的机器，可以通过命令 `qemu-system-riscv64 -machine help`查看可选择的机器选项
- `-kernel`: 指定内核image
- `-append cmdline`: 使用cmdline作为内核的命令行
- `-device`: 指定要模拟的设备，可以通过命令 `qemu-system-riscv64 -device help` 查看可选择的设备，通过命令 `qemu-system-riscv64 -device <具体的设备>,help` 查看某个设备的命令选项
- `-drive, file=<file_name>`: 使用 `file_name` 作为文件系统
- `-S`: 启动时暂停CPU执行
- `-s`: -gdb tcp::1234 的简写
- `-bios default`: 使用默认的 OpenSBI firmware 作为 bootloader

更多参数信息可以参考[这里](https://www.qemu.org/docs/master/system/index.html)

### 3.4 GDB 使用基础

#### 什么是 GDB

GNU 调试器（英语：GNU Debugger，缩写：gdb）是一个由 GNU 开源组织发布的、UNIX/LINUX操作系统下的、基于命令行的、功能强大的程序调试工具。借助调试器，我们能够查看另一个程序在执行时实际在做什么（比如访问哪些内存、寄存器），在其他程序崩溃的时候可以比较快速地了解导致程序崩溃的原因。
被调试的程序可以是和gdb在同一台机器上（本地调试，or native debug），也可以是不同机器上（远程调试， or remote debug）。

总的来说，gdb可以有以下4个功能：

- 启动程序，并指定可能影响其行为的所有内容
- 使程序在指定条件下停止
- 检查程序停止时发生了什么
- 更改程序中的内容，以便纠正一个bug的影响

#### GDB 基本命令介绍

- (gdb) layout asm: 显示汇编代码
- (gdb) start: 单步执行，运行程序，停在第一执行语句
- (gdb) continue: 从断点后继续执行，简写 `c`
- (gdb) next: 单步调试（逐过程，函数直接执行），简写 `n`
- (gdb) step instruction: 执行单条指令，简写 `si`
- (gdb) run: 重新开始运行文件（run-text：加载文本文件，run-bin：加载二进制文件），简写 `r`
- (gdb) backtrace：查看函数的调用的栈帧和层级关系，简写 `bt`
- (gdb) break 设置断点，简写 `b`
    - 断在 `foo` 函数：`b foo`
    - 断在某地址: `b * 0x80200000`
- (gdb) finish: 结束当前函数，返回到函数调用点
- (gdb) frame: 切换函数的栈帧，简写 `f`
- (gdb) print: 打印值及地址，简写 `p`
- (gdb) info：查看函数内部局部变量的数值，简写 `i`
    - 查看寄存器 ra 的值：`i r ra`
- (gdb) display：追踪查看具体变量值
- (gdb) x/4x <addr>: 以 16 进制打印 <addr> 处开始的 16 Bytes 内容

更多命令可以参考[100个gdb小技巧](https://wizardforcel.gitbooks.io/100-gdb-tips/content/)


### 3.5 Linux 内核编译基础

#### 交叉编译

交叉编译指的是在一个平台上编译可以在另一个架构运行的程序。例如在 x86 机器上编译可以在 RISC-V 架构运行的程序，交叉编译需要交叉编译工具链的支持，在我们的实验中所用的交叉编译工具链就是 `riscv-gnu-toolchain`。

#### 内核配置

内核配置是用于配置是否启用内核的各项特性，内核会提供一个名为 `defconfig` (即default configuration) 的默认配置，该配置文件位于各个架构目录的 `configs` 文件夹下，例如对于RISC-V而言，其默认配置文件为 `arch/riscv/configs/defconfig`。使用 `make ARCH=riscv defconfig` 命令可以在内核根目录下生成一个名为 `.config` 的文件，包含了内核完整的配置，内核在编译时会根据 `.config` 进行编译。配置之间存在相互的依赖关系，直接修改defconfig文件或者 `.config` 有时候并不能达到想要的效果。因此如果需要修改配置一般采用 `make ARCH=riscv menuconfig` 的方式对内核进行配置。

#### 常见参数

- `ARCH` 指定架构，可选的值包括arch目录下的文件夹名，如 x86、arm、arm64 等，不同于 arm 和 arm64，32 位和 64 位的RISC-V共用 `arch/riscv` 目录，通过使用不同的 config 可以编译 32 位或 64 位的内核。
- `CROSS_COMPILE` 指定使用的交叉编译工具链，例如指定 `CROSS_COMPILE=riscv64-unknown-linux-gnu-`，则编译时会采用 `riscv64-unknown-linux-gnu-gcc` 作为编译器，编译可以在 RISC-V 64 位平台上运行的 kernel。

#### 常用的 Linux 下的编译选项

```bash
$ make help         # 查看make命令的各种参数解释

$ make defconfig    # 使用当前平台的默认配置，在x86机器上会使用x86的默认配置
$ make -j$(nproc)   # 编译当前平台的内核，-j$(nproc) 为以全部机器硬件线程数进行多线程编译
$ make -j4          # 编译当前平台的内核，-j4 为使用 4 线程进行多线程编译

$ make ARCH=riscv defconfig                                     # 使用 RISC-V 平台的默认配置
$ make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)   # 编译 RISC-V 平台内核

$ make clean        # 清除所有编译好的 object 文件
```

## 4 实验步骤

**在执行每一条命令前，请你对将要进行的操作进行思考，给出的命令不需要全部执行，并且不是所有的命令都可以无条件执行，请不要直接复制粘贴命令去执行。**

### 4.1 搭建 Docker 环境

请根据 **3.2 Docker 使用基础** 安装 Docker 环境。然后**参考并理解**以下步骤，导入我们已经准备好的 Docker 镜像：

```bash
# 导入docker镜像
$ cat oslab.tar | docker import - oslab:2021

# 查看docker镜像
$ docker images
REPOSITORY   TAG       IMAGE ID       CREATED        SIZE
oslab        latest    b2b39a3bcd81   404 days ago   3.62GB

# 从镜像创建一个容器
$ docker run --name oslab -it oslab:2021 bash   # --name:容器名称 -i:交互式操作 -t:终端
root@132a140bd724:/#                            # 提示符变为 '#' 表明成功进入容器 后面的字符串根据容器而生成，为容器id
root@132a140bd724:/# exit (or CTRL+D)           # 从容器中退出 此时运行docker ps，运行容器的列表为空

# 启动处于停止状态的容器
$ docker start oslab        # oslab为容器名称
$ docker ps                 # 可看到容器已经启动
CONTAINER ID   IMAGE        COMMAND       CREATED              STATUS        PORTS     NAMES
132a140bd724   oslab:2021   "bash"        About a minute ago   Up 1 second             oslab

# 从终端连入 docker 容器
$ docker exec -it oslab bash

# 挂载本地目录
# 把用户的 home 目录映射到 docker 镜像内的 have-fun-debugging 目录
$ docker run --name oslab -it -v ${HOME}:/have-fun-debugging oslab:2021 bash    # -v 本地目录:容器内目录
```

### 4.2 获取 Linux 源码和已经编译好的文件系统

从 [https://www.kernel.org](https://www.kernel.org) 下载最新的 Linux 源码。

并且使用 git 工具 clone [本仓库](https://gitee.com/zjusec/os21fall)。其中已经准备好了根文件系统的镜像。

> 根文件系统为 Linux Kenrel 提供了基础的文件服务，在启动 Linux Kernel 时是必要的。

```bash
$ git clone https://gitee.com/zjusec/os21fall
$ cd os21fall/src/lab0
$ ls
rootfs.img  # 已经构建完成的根文件系统的镜像
```

### 4.3 编译 linux 内核

```bash
$ pwd
path/to/lab0/linux
$ make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- defconfig    # 生成配置
$ make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- -j$(nproc)   # 编译
```

> 使用多线程编译一般会耗费大量内存，如果 `-j` 选项导致内存耗尽 (out of memory)，请尝试调低线程数，比如 `-j4`, `-j8` 等。

### 4.4 使用QEMU运行内核

```bash
$ qemu-system-riscv64 -nographic -machine virt -kernel path/to/linux/arch/riscv/boot/Image \
    -device virtio-blk-device,drive=hd0 -append "root=/dev/vda ro console=ttyS0" \
    -bios default -drive file=rootfs.img,format=raw,id=hd0
```
退出 QEMU 的方法为：使用 Ctrl+A，**松开**后再按下 X 键即可退出 QEMU。

### 4.5 使用 GDB 对内核进行调试

这一步需要开启两个 Terminal Session，一个 Terminal 使用 QEMU 启动 Linux，另一个 Terminal 使用 GDB 与 QEMU 远程通信（使用 tcp::1234 端口）进行调试。

```bash
# Terminal 1
$ qemu-system-riscv64 -nographic -machine virt -kernel path/to/linux/arch/riscv/boot/Image \
    -device virtio-blk-device,drive=hd0 -append "root=/dev/vda ro console=ttyS0" \
    -bios default -drive file=rootfs.img,format=raw,id=hd0 -S -s

# Terminal 2
$ riscv64-unknown-linux-gnu-gdb path/to/linux/vmlinux
(gdb) target remote :1234   # 连接 qemu
(gdb) b start_kernel        # 设置断点
(gdb) continue              # 继续执行
(gdb) quit                  # 退出 gdb
```

## 5 实验任务与要求

- 请各位同学独立完成作业，任何抄袭行为都将使本次作业判为0分。
- 编译内核并用 GDB + QEMU 调试，在内核初始化过程中设置断点，对内核的启动过程进行跟踪，并尝试使用gdb的各项命令（如backtrace、finish、frame、info、break、display、next、layout等）。
- 在学在浙大中提交 pdf 格式的实验报告，记录实验过程并截图（4.1-4.4）.，对每一步的命令以及结果进行必要的解释，记录遇到的问题和心得体会。

## 思考题

1. 使用 `riscv64-unknown-elf-gcc` 编译单个 `.c` 文件
2. 使用 `riscv64-unknown-elf-objdump` 反汇编 1 中得到的编译产物
3. 调试 Linux 时:
    1. 在 GDB 中查看汇编代码
    2. 在 0x80000000 处下断点
    3. 查看所有已下的断点
    4. 在 0x80200000 处下断点
    5. 清除 0x80000000 处的断点
    6. 继续运行直到触发 0x80200000 处的断点
    7. 单步调试一次
    8. 退出 QEMU
4. 使用 `make` 工具清除 Linux 的构建产物
5. `vmlinux` 和 `Image` 的关系和区别是什么？