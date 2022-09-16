/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief x86 (INTEL64) specific syscall header
 *
 * This header contains the x86 specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL64_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL64_SYSCALL_H_

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * x86_64 System V calling convention:
 *   First six arguments passed in via RDI, RSI, RDX, RCX, R8, R9
 *   We'll use RAX for the call_id, and the return value
 *
 * Arrange registers so that they are in-place as much as possible when
 * doing the system call. Because RCX get overwritten by the CPU, put arg 4
 * in r10 instead.
 *
 * SYSCALL instruction stores return address in RCX and RFLAGS in R11.  RIP is
 * loaded from LSTAR MSR, masks RFLAGS with the low 32 bits of EFER.SFMASK. CS
 * and SS are loaded from values derived from bits 47:32 of STAR MSR (+0
 * for CS, +8 for SS)
 *
 * SYSRET loads RIP from RCX and RFLAGS from r11. CS and SS are set with
 * values derived from STAR MSR bits 63:48 (+8 for CS, +16 for SS)
 *
 * The kernel is in charge of not clobbering across the system call
 * the remaining registers: RBX, RBP, R12-R15, SIMD/FPU, and any unused
 * argument registers.
 */
static inline uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id)
{
	register uintptr_t rax __asm__("%rax") = call_id;
	register uintptr_t rdi __asm__("%rdi") = arg1;
	register uintptr_t rsi __asm__("%rsi") = arg2;
	register uintptr_t rdx __asm__("%rdx") = arg3;
	register uintptr_t r10 __asm__("%r10") = arg4; /* RCX unavailable */
	register uintptr_t r8 __asm__("%r8") = arg5;
	register uintptr_t r9 __asm__("%r9") = arg6;

	__asm__ volatile("syscall\n\t"
			 : "=r" (rax)
			 : "r" (rax), "r" (rdi), "r" (rsi), "r" (rdx),
			   "r" (r10), "r" (r8), "r" (r9)
			 : "memory", "rcx", "r11");

	return rax;
}

static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5,
					     uintptr_t call_id)
{
	register uintptr_t rax __asm__("%rax") = call_id;
	register uintptr_t rdi __asm__("%rdi") = arg1;
	register uintptr_t rsi __asm__("%rsi") = arg2;
	register uintptr_t rdx __asm__("%rdx") = arg3;
	register uintptr_t r10 __asm__("%r10") = arg4; /* RCX unavailable */
	register uintptr_t r8 __asm__("%r8") = arg5;

	__asm__ volatile("syscall\n\t"
			 : "=r" (rax)
			 : "r" (rax), "r" (rdi), "r" (rsi), "r" (rdx),
			   "r" (r10), "r" (r8)
			 : "memory", "rcx", "r11");

	return rax;
}

static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id)
{
	register uintptr_t rax __asm__("%rax") = call_id;
	register uintptr_t rdi __asm__("%rdi") = arg1;
	register uintptr_t rsi __asm__("%rsi") = arg2;
	register uintptr_t rdx __asm__("%rdx") = arg3;
	register uintptr_t r10 __asm__("%r10") = arg4; /* RCX unavailable */

	__asm__ volatile("syscall\n\t"
			 : "=r" (rax)
			 : "r" (rax), "r" (rdi), "r" (rsi), "r" (rdx),
			   "r" (r10)
			 : "memory", "rcx", "r11");

	return rax;
}

static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3,
					     uintptr_t call_id)
{
	register uintptr_t rax __asm__("%rax") = call_id;
	register uintptr_t rdi __asm__("%rdi") = arg1;
	register uintptr_t rsi __asm__("%rsi") = arg2;
	register uintptr_t rdx __asm__("%rdx") = arg3;

	__asm__ volatile("syscall\n\t"
			 : "=r" (rax)
			 : "r" (rax), "r" (rdi), "r" (rsi), "r" (rdx)
			 : "memory", "rcx", "r11");

	return rax;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)

{
	register uintptr_t rax __asm__("%rax") = call_id;
	register uintptr_t rdi __asm__("%rdi") = arg1;
	register uintptr_t rsi __asm__("%rsi") = arg2;

	__asm__ volatile("syscall\n\t"
			 : "=r" (rax)
			 : "r" (rax), "r" (rdi), "r" (rsi)
			 : "memory", "rcx", "r11");

	return rax;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1,
					     uintptr_t call_id)
{
	register uintptr_t rax __asm__("%rax") = call_id;
	register uintptr_t rdi __asm__("%rdi") = arg1;

	__asm__ volatile("syscall\n\t"
			 : "=r" (rax)
			 : "r" (rax), "r" (rdi)
			 : "memory", "rcx", "r11");

	return rax;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register uintptr_t rax __asm__("%rax") = call_id;

	__asm__ volatile("syscall\n\t"
			 : "=r" (rax)
			 : "r" (rax)
			 : "memory", "rcx", "r11");

	return rax;
}

static inline bool arch_is_user_context(void)
{
	int cs;

	__asm__ volatile ("mov %%cs, %[cs_val]" : [cs_val] "=r" (cs));

	return (cs & 0x3) != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_SYSCALL_H_ */
