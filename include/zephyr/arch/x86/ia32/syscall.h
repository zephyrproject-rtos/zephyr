/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief x86 (IA32) specific syscall header
 *
 * This header contains the x86 specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_IA32_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_X86_IA32_SYSCALL_H_

#define USER_CODE_SEG	0x2b /* at dpl=3 */
#define USER_DATA_SEG	0x33 /* at dpl=3 */

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/linker/sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Syscall invocation macros. x86-specific machine constraints used to ensure
 * args land in the proper registers, see implementation of
 * z_x86_syscall_entry_stub in userspace.S
 */

__pinned_func
static inline uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id)
{
	uint32_t ret;

	__asm__ volatile("push %%ebp\n\t"
			 "mov %[arg6], %%ebp\n\t"
			 "int $0x80\n\t"
			 "pop %%ebp\n\t"
			 : "=a" (ret)
			 : "S" (call_id), "a" (arg1), "d" (arg2),
			   "c" (arg3), "b" (arg4), "D" (arg5),
			   [arg6] "m" (arg6)
			 : "memory");
	return ret;
}

__pinned_func
static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5,
					     uintptr_t call_id)
{
	uint32_t ret;

	__asm__ volatile("int $0x80"
			 : "=a" (ret)
			 : "S" (call_id), "a" (arg1), "d" (arg2),
			   "c" (arg3), "b" (arg4), "D" (arg5)
			 : "memory");
	return ret;
}

__pinned_func
static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id)
{
	uint32_t ret;

	__asm__ volatile("int $0x80"
			 : "=a" (ret)
			 : "S" (call_id), "a" (arg1), "d" (arg2), "c" (arg3),
			   "b" (arg4)
			 : "memory");
	return ret;
}

__pinned_func
static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3,
					     uintptr_t call_id)
{
	uint32_t ret;

	__asm__ volatile("int $0x80"
			 : "=a" (ret)
			 : "S" (call_id), "a" (arg1), "d" (arg2), "c" (arg3)
			 : "memory");
	return ret;
}

__pinned_func
static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)
{
	uint32_t ret;

	__asm__ volatile("int $0x80"
			 : "=a" (ret)
			 : "S" (call_id), "a" (arg1), "d" (arg2)
			 : "memory"
			 );
	return ret;
}

__pinned_func
static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1,
					     uintptr_t call_id)
{
	uint32_t ret;

	__asm__ volatile("int $0x80"
			 : "=a" (ret)
			 : "S" (call_id), "a" (arg1)
			 : "memory"
			 );
	return ret;
}

__pinned_func
static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	uint32_t ret;

	__asm__ volatile("int $0x80"
			 : "=a" (ret)
			 : "S" (call_id)
			 : "memory"
			 );
	return ret;
}

__pinned_func
static inline bool arch_is_user_context(void)
{
	int cs;

	/* On x86, read the CS register (which cannot be manually set) */
	__asm__ volatile ("mov %%cs, %[cs_val]" : [cs_val] "=r" (cs));

	return cs == USER_CODE_SEG;
}


#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_INCLUDE_ARCH_X86_IA32_SYSCALL_H_ */
