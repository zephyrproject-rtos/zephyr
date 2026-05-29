/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa specific syscall header
 *
 * This header contains the Xtensa specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_SYSCALL_H_

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util_macro.h>

#include <xtensa/config/core-isa.h>

#ifdef __cplusplus
extern "C" {
#endif

/* When syscall assembly is executed, the EPC points to the syscall
 * instruction, and we have to manually advance it so we will
 * return to the instruction after syscall to continue execution.
 * However, with zero-overhead loops and the syscall instruction is
 * the last instruction, this simple addition does not work as it
 * would point past the loop and would have skipped the loop.
 * Because of this, syscall entrance would need to look at the loop
 * registers and set the PC back to the beginning of loop if we are
 * still looping. Assuming most of the syscalls are not inside
 * loops, the extra handling code consumes quite a few cycles.
 * To workaround this, simply adds a nop after syscall so we no
 * longer have to deal with loops at syscall entrance, and that
 * a nop is faster than all the code to manipulate loop registers.
 */
#ifdef XCHAL_HAVE_LOOPS
#define XTENSA_SYSCALL_ASM		"syscall; nop;"
#else
#define XTENSA_SYSCALL_ASM		"syscall"
#endif

#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER
uintptr_t xtensa_syscall_helper_args_6(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t arg4,
				       uintptr_t arg5, uintptr_t arg6,
				       uintptr_t call_id);

uintptr_t xtensa_syscall_helper_args_5(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t arg4,
				       uintptr_t arg5, uintptr_t call_id);

uintptr_t xtensa_syscall_helper_args_4(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t arg4,
				       uintptr_t call_id);

#define SYSINL ALWAYS_INLINE
#else
#define SYSINL inline
#endif /* CONFIG_XTENSA_SYSCALL_USE_HELPER */

/**
 * We are following Linux Xtensa syscall ABI:
 *
 *  syscall number     arg1, arg2, arg3, arg4, arg5, arg6
 *  --------------     ----------------------------------
 *  a2                 a6,   a3,   a4,   a5,   a8,   a9
 *
 **/


static SYSINL uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id)
{
#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER
	return xtensa_syscall_helper_args_6(arg1, arg2, arg3, arg4, arg5, arg6, call_id);
#else
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;
	register uintptr_t a8 __asm__("%a8") = arg5;
	register uintptr_t a9 __asm__("%a9") = arg6;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5), "r" (a8), "r" (a9)
			 : "memory");

	return a2;
#endif /* CONFIG_XTENSA_SYSCALL_USE_HELPER */
}

static SYSINL uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t call_id)
{
#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER
	return xtensa_syscall_helper_args_5(arg1, arg2, arg3, arg4, arg5, call_id);
#else
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;
	register uintptr_t a8 __asm__("%a8") = arg5;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5), "r" (a8)
			 : "memory");

	return a2;
#endif /* CONFIG_XTENSA_SYSCALL_USE_HELPER */
}

static SYSINL uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id)
{
#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER
	return xtensa_syscall_helper_args_4(arg1, arg2, arg3, arg4, call_id);
#else
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5)
			 : "memory");

	return a2;
#endif /* CONFIG_XTENSA_SYSCALL_USE_HELPER */
}

static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4)
			 : "memory");

	return a2;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3)
			 : "memory");

	return a2;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6)
			 : "memory");

	return a2;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2)
			 : "memory");

	return a2;
}

/*
 * There is no easy (or generic) way to figure out if a thread is runnining
 * in un-privileged mode. Reading the current ring (PS.CRING) is a privileged
 * instruction and not thread local storage is not available in xcc.
 */
static inline bool arch_is_user_context(void)
{
#if XCHAL_HAVE_THREADPTR
	uint32_t thread;

	__asm__ volatile(
		"rur.THREADPTR %0\n\t"
		: "=a" (thread)
	);
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	extern Z_THREAD_LOCAL uint32_t is_user_mode;

	if (!thread) {
		return false;
	}

	return is_user_mode != 0;
#else
	return !!thread;
#endif

#else /* XCHAL_HAVE_THREADPTR */
	extern bool xtensa_is_user_context(void);

	return xtensa_is_user_context();
#endif /* XCHAL_HAVE_THREADPTR */
}

#undef SYSINL

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_SYSCALL_H_ */
