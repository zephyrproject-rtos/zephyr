/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OpenRISC specific syscall header
 *
 * This header contains the OpenRISC specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_OPENRISC_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_OPENRISC_SYSCALL_H_

/*
 * Privileged mode system calls
 */
#define OR_SYSCALL_RUNTIME_EXCEPT	0
#define OR_SYSCALL_IRQ_OFFLOAD		1

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Syscall invocation macros. OpenRISC-specific machine constraints used to
 * ensure args land in the proper registers.
 */
static inline uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id)
{
	register unsigned long r3 __asm__ ("r3") = arg1;
	register unsigned long r4 __asm__ ("r4") = arg2;
	register unsigned long r5 __asm__ ("r5") = arg3;
	register unsigned long r6 __asm__ ("r6") = arg4;
	register unsigned long r7 __asm__ ("r7") = arg5;
	register unsigned long r8 __asm__ ("r8") = arg6;
	register unsigned long r11 __asm__ ("r11") = call_id;

	__asm__ volatile ("l.sys 0"
			  : "+r" (r11)
			  : "r" (r3), "r" (r4), "r" (r5), "r" (r6), "r" (r7),
			  "r" (r8)
			  : "memory");
	return r11;
}

static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5,
					     uintptr_t call_id)
{
	register unsigned long r3 __asm__ ("r3") = arg1;
	register unsigned long r4 __asm__ ("r4") = arg2;
	register unsigned long r5 __asm__ ("r5") = arg3;
	register unsigned long r6 __asm__ ("r6") = arg4;
	register unsigned long r7 __asm__ ("r7") = arg5;
	register unsigned long r11 __asm__ ("r11") = call_id;

	__asm__ volatile ("l.sys 0"
			  : "+r" (r11)
			  : "r" (r3), "r" (r4), "r" (r5), "r" (r6), "r" (r7)
			  : "memory");
	return r11;
}

static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id)
{
	register unsigned long r3 __asm__ ("r3") = arg1;
	register unsigned long r4 __asm__ ("r4") = arg2;
	register unsigned long r5 __asm__ ("r5") = arg3;
	register unsigned long r6 __asm__ ("r6") = arg4;
	register unsigned long r11 __asm__ ("r11") = call_id;

	__asm__ volatile ("l.sys 0"
			  : "+r" (r11)
			  : "r" (r3), "r" (r4), "r" (r5), "r" (r6)
			  : "memory");
	return r11;
}

static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3,
					     uintptr_t call_id)
{
	register unsigned long r3 __asm__ ("r3") = arg1;
	register unsigned long r4 __asm__ ("r4") = arg2;
	register unsigned long r5 __asm__ ("r5") = arg3;
	register unsigned long r11 __asm__ ("r11") = call_id;

	__asm__ volatile ("l.sys 0"
			  : "+r" (r11)
			  : "r" (r3), "r" (r4), "r" (r5)
			  : "memory");
	return r11;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)
{
	register unsigned long r3 __asm__ ("r3") = arg1;
	register unsigned long r4 __asm__ ("r4") = arg2;
	register unsigned long r11 __asm__ ("r11") = call_id;

	__asm__ volatile ("l.sys 0"
			  : "+r" (r11)
			  : "r" (r3), "r" (r4)
			  : "memory");
	return r11;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1, uintptr_t call_id)
{
	register unsigned long r3 __asm__ ("r3") = arg1;
	register unsigned long r11 __asm__ ("r11") = call_id;

	__asm__ volatile ("l.sys 0"
			  : "+r" (r11)
			  : "r" (r3)
			  : "memory");
	return r11;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register unsigned long r11 __asm__ ("r11") = call_id;

	__asm__ volatile ("l.sys 0"
			  : "+r" (r11)
			  :
			  : "memory");
	return r11;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_OPENRISC_SYSCALL_H_ */
