# 常见问题及解答

- [Linux 源码放在共享文件夹下编译失败](#1)


## 1 Linux 源码放在共享文件夹下编译失败

这种情况请使用 `wget` 等工具将 Linux 源码下载至容器内目录**而非共享目录**，然后执行编译。

## 2 QEMU+GDB 使用 `si` 单指令调试遇到模式切换失效

在遇到诸如 `mret`, `sret` 等指令造成的模式切换时，`si` 指令会失效，可能表现为程序开始不停跑，影响对程序运行行为的判断。

一个解决方法是在程序**预期跳转**的位置打上断点，断点不会受到模式切换的影响，比如：

```bash
(gdb) i r sepc    
sepc        0x8000babe
(gdb) b * 0x8000babe
Breakpoint 1 at 0x8000babe
(gdb) si    # 或者使用 c
Breakpoint 1, 0x000000008000babe in _never_gonna_give_you_up ()
...
```

这样就可以看到断点被触发，可以继续调试了。