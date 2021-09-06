# Lab 0: 环境搭建

## 实验目的

配置开发环境，通过在 QEMU 上运行 Linux 来熟悉如何从源代码开始将内核运行在 QEMU 模拟器上，并且掌握使用 GDB 跟 QEMU 进行联合调试，为后续实验打下基础。

## 实验背景

### Linux 使用基础

操作系统是一门注重实践的课程，我们往往要精通许多工具的使用，才能在复杂的实验中游刃有余。

**强烈建议**在正式进行操作系统的学习之前，自学 Shell, Git, Docker, Markdown 等一系列基本工具的使用。如果你不知道该如何入手，可以参考 [The Missing Semester of Your CS Education](https://missing-semester-cn.github.io/)

### Docker

TODO

### QEMU

TODO



## 实验步骤

### 1. 安装 Docker

### 2. 下载 Docker Image


### 3. 从 Docker Image 进入 Container


### 4. 编译 Linux


### 5. 运行 Linux


### 6. 使用 GDB 调试 Linux



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
4. 使用 `make` 命令清除 Linux 的构建产物