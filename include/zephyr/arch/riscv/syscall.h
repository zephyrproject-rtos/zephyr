/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISCV specific syscall header
 *
 * This header contains the RISCV specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_SYSCALL_H_

/*
 * Privileged mode system calls
 */
#define RV_ECALL_RUNTIME_EXCEPT		0
#define RV_ECALL_IRQ_OFFLOAD		1

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Syscall invocation macros. riscv-specific machine constraints used to ensure
 * args land in the proper registers.
 */
static inline uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id)
{
	register ulong_t a0 __asm__ ("a0") = arg1;
	register ulong_t a1 __asm__ ("a1") = arg2;
	register ulong_t a2 __asm__ ("a2") = arg3;
	register ulong_t a3 __asm__ ("a3") = arg4;
	register ulong_t a4 __asm__ ("a4") = arg5;
	register ulong_t a5 __asm__ ("a5") = arg6;
	register ulong_t t0 __asm__ ("t0") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5),
			  "r" (t0)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5,
					     uintptr_t call_id)
{
	register ulong_t a0 __asm__ ("a0") = arg1;
	register ulong_t a1 __asm__ ("a1") = arg2;
	register ulong_t a2 __asm__ ("a2") = arg3;
	register ulong_t a3 __asm__ ("a3") = arg4;
	register ulong_t a4 __asm__ ("a4") = arg5;
	register ulong_t t0 __asm__ ("t0") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (t0)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id)
{
	register ulong_t a0 __asm__ ("a0") = arg1;
	register ulong_t a1 __asm__ ("a1") = arg2;
	register ulong_t a2 __asm__ ("a2") = arg3;
	register ulong_t a3 __asm__ ("a3") = arg4;
	register ulong_t t0 __asm__ ("t0") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a2), "r" (a3), "r" (t0)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3,
					     uintptr_t call_id)
{
	register ulong_t a0 __asm__ ("a0") = arg1;
	register ulong_t a1 __asm__ ("a1") = arg2;
	register ulong_t a2 __asm__ ("a2") = arg3;
	register ulong_t t0 __asm__ ("t0") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a2), "r" (t0)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)
{
	register ulong_t a0 __asm__ ("a0") = arg1;
	register ulong_t a1 __asm__ ("a1") = arg2;
	register ulong_t t0 __asm__ ("t0") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (t0)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1, uintptr_t call_id)
{
	register ulong_t a0 __asm__ ("a0") = arg1;
	register ulong_t t0 __asm__ ("t0") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (t0)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register ulong_t a0 __asm__ ("a0");
	register ulong_t t0 __asm__ ("t0") = call_id;

	__asm__ volatile ("ecall"
			  : "=r" (a0)
			  : "r" (t0)
			  : "memory");
	return a0;
}

#ifdef CONFIG_USERSPACE
static inline bool arch_is_user_context(void)
{
#ifdef CONFIG_SMP
	/*
	 * This is painful. There is no way for u-mode code to know if we're
	 * currently executing in u-mode without generating a fault, besides
	 * stealing a general purpose register away from the standard ABI
	 * that is. And a global variable doesn't work on SMP as this must be
	 * per-CPU and we could be migrated to another CPU just at the right
	 * moment to peek at the wrong CPU variable (and u-mode can't disable
	 * preemption either).
	 *
	 * So, given that we'll have to pay the price of an exception entry
	 * anyway, let's at least make it free to privileged threads by using
	 * the mscratch register as the non-user context indicator (it must
	 * be zero in m-mode for exception entry to work properly). In the
	 * case of u-mode we'll simulate a proper return value in the
	 * exception trap code. Let's settle on the return value in t0
	 * and omit the volatile to give the compiler a chance to cache
	 * the result.
	 */
	register ulong_t is_user __asm__ ("t1");
	__asm__ ("csrr %0, mscratch" : "=r" (is_user));
	return is_user != 0;
#else
	/* Defined in arch/riscv/core/thread.c */
	extern uint32_t is_user_mode;
	return is_user_mode;
#endif
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_SYSCALL_H_ */
