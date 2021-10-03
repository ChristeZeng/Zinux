#include "types.h"
#include "sbi.h"


struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{
    struct sbiret ret;

	asm volatile (
        "mv a0, %[arg0]\n"
		"mv a1, %[arg1]\n"
		"mv a2, %[arg2]\n"
		"mv a3, %[arg3]\n"
		"mv a4, %[arg4]\n"
        "mv a5, %[arg5]\n"
		"mv a6, %[fid]\n"
		"mv a7, %[ext]\n"
        "ecall\n"
		"mv %[error], a0\n"
		"mv %[value], a1"
        : [error] "=r" (ret.error), [value] "=r" (ret.value)
		: [arg0] "r" (arg0), [arg1] "r" (arg1), [arg2] "r" (arg2), [arg3] "r" (arg3), [arg4] "r" (arg4), [arg5] "r" (arg5), [ext] "r" (ext), [fid] "r" (fid)
		: "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7" 
	);
	// register uint64 a0 asm ("a0") = (uint64)(arg0);
	// register uint64 a1 asm ("a1") = (uint64)(arg1);
	// register uint64 a2 asm ("a2") = (uint64)(arg2);
	// register uint64 a3 asm ("a3") = (uint64)(arg3);
	// register uint64 a4 asm ("a4") = (uint64)(arg4);
	// register uint64 a5 asm ("a5") = (uint64)(arg5);
	// register uint64 a6 asm ("a6") = (uint64)(fid);
	// register uint64 a7 asm ("a7") = (uint64)(ext);
	// asm volatile ("ecall"
	// 	      : "+r" (a0), "+r" (a1)
	// 	      : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
	// 	      : "memory");
	// ret.error = a0;
	// ret.value = a1;

	return ret;         
}
