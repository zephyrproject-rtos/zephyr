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

#define _SVC_CALL_CONTEXT_SWITCH	0
#define _SVC_CALL_IRQ_OFFLOAD		1
#define _SVC_CALL_RUNTIME_EXCEPT	2
#define _SVC_CALL_SYSTEM_CALL		3

#define FORCE_SYSCALL_ID		-1

#ifdef CONFIG_USERSPACE
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
	register uint32_t a0 __asm__ ("a0") = arg1;
	register uint32_t a1 __asm__ ("a1") = arg2;
	register uint32_t a2 __asm__ ("a2") = arg3;
	register uint32_t a3 __asm__ ("a3") = arg4;
	register uint32_t a4 __asm__ ("a4") = arg5;
	register uint32_t a5 __asm__ ("a5") = arg6;
	register uint32_t a7 __asm__ ("a7") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5),
			  "r" (a7)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5,
					     uintptr_t call_id)
{
	register uint32_t a0 __asm__ ("a0") = arg1;
	register uint32_t a1 __asm__ ("a1") = arg2;
	register uint32_t a2 __asm__ ("a2") = arg3;
	register uint32_t a3 __asm__ ("a3") = arg4;
	register uint32_t a4 __asm__ ("a4") = arg5;
	register uint32_t a7 __asm__ ("a7") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a7)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id)
{
	register uint32_t a0 __asm__ ("a0") = arg1;
	register uint32_t a1 __asm__ ("a1") = arg2;
	register uint32_t a2 __asm__ ("a2") = arg3;
	register uint32_t a3 __asm__ ("a3") = arg4;
	register uint32_t a7 __asm__ ("a7") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a2), "r" (a3), "r" (a7)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3,
					     uintptr_t call_id)
{
	register uint32_t a0 __asm__ ("a0") = arg1;
	register uint32_t a1 __asm__ ("a1") = arg2;
	register uint32_t a2 __asm__ ("a2") = arg3;
	register uint32_t a7 __asm__ ("a7") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a2), "r" (a7)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)
{
	register uint32_t a0 __asm__ ("a0") = arg1;
	register uint32_t a1 __asm__ ("a1") = arg2;
	register uint32_t a7 __asm__ ("a7") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a1), "r" (a7)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1, uintptr_t call_id)
{
	register uint32_t a0 __asm__ ("a0") = arg1;
	register uint32_t a7 __asm__ ("a7") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a7)
			  : "memory");
	return a0;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register uint32_t a0 __asm__ ("a0");
	register uint32_t a7 __asm__ ("a7") = call_id;

	__asm__ volatile ("ecall"
			  : "+r" (a0)
			  : "r" (a7)
			  : "memory");
	return a0;
}

static inline bool arch_is_user_context(void)
{
	/* Defined in arch/riscv/core/thread.c */
	extern ulong_t is_user_mode;
	return is_user_mode;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_SYSCALL_H_ */
