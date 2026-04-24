/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore specific syscall header
 *
 * This header contains the TriCore specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_SYSCALL_H_

/* Syscall ids used */
#define _SYSCALL_CALL 0
#define _SYSCALL_SWITCH 1
#define _SYSCALL_EXCEPT 2

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Syscall invocation macros. tricore-specific machine constraints used to ensure
 * args land in the proper registers.
 */
static inline uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
					     uintptr_t arg4, uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id)
{
	register unsigned long d4 __asm__("d4") = arg1;
	register unsigned long d5 __asm__("d5") = arg2;
	register unsigned long d6 __asm__("d6") = arg3;
	register unsigned long d7 __asm__("d7") = arg4;
	register unsigned long d8 __asm__("d8") = arg5;
	register unsigned long d9 __asm__("d9") = arg6;
	register unsigned long d0 __asm__("d0") = call_id;
	register unsigned long d2 __asm__("d2");

	__asm__ volatile("syscall 0"
			 : "=r"(d2)
			 : "r"(d4), "r"(d5), "r"(d6), "r"(d7), "r"(d8), "r"(d9), "r"(d0)
			 : "memory");
	return d2;
}

static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
					     uintptr_t arg4, uintptr_t arg5, uintptr_t call_id)
{
	register unsigned long d4 __asm__("d4") = arg1;
	register unsigned long d5 __asm__("d5") = arg2;
	register unsigned long d6 __asm__("d6") = arg3;
	register unsigned long d7 __asm__("d7") = arg4;
	register unsigned long d8 __asm__("d8") = arg5;
	register unsigned long d0 __asm__("d0") = call_id;
	register unsigned long d2 __asm__("d2");

	__asm__ volatile("syscall 0"
			 : "=r"(d2)
			 : "r"(d4), "r"(d5), "r"(d6), "r"(d7), "r"(d8), "r"(d0)
			 : "memory");
	return d2;
}

static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
					     uintptr_t arg4, uintptr_t call_id)
{
	register unsigned long d4 __asm__("d4") = arg1;
	register unsigned long d5 __asm__("d5") = arg2;
	register unsigned long d6 __asm__("d6") = arg3;
	register unsigned long d7 __asm__("d7") = arg4;
	register unsigned long d0 __asm__("d0") = call_id;
	register unsigned long d2 __asm__("d2");

	__asm__ volatile("syscall 0"
			 : "=r"(d2)
			 : "r"(d4), "r"(d5), "r"(d6), "r"(d7), "r"(d0)
			 : "memory");
	return d2;
}

static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
					     uintptr_t call_id)
{
	register unsigned long d4 __asm__("d4") = arg1;
	register unsigned long d5 __asm__("d5") = arg2;
	register unsigned long d6 __asm__("d6") = arg3;
	register unsigned long d0 __asm__("d0") = call_id;
	register unsigned long d2 __asm__("d2");

	__asm__ volatile("syscall 0" : "=r"(d2) : "r"(d4), "r"(d5), "r"(d6), "r"(d0) : "memory");
	return d2;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2, uintptr_t call_id)
{
	register unsigned long d4 __asm__("d4") = arg1;
	register unsigned long d5 __asm__("d5") = arg2;
	register unsigned long d0 __asm__("d0") = call_id;
	register unsigned long d2 __asm__("d2");

	__asm__ volatile("syscall 0" : "=r"(d2) : "r"(d4), "r"(d5), "r"(d0) : "memory");
	return d2;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1, uintptr_t call_id)
{
	register unsigned long d4 __asm__("d4") = arg1;
	register unsigned long d0 __asm__("d0") = call_id;
	register unsigned long d2 __asm__("d2");

	__asm__ volatile("syscall 0" : "=r"(d2) : "r"(d4), "r"(d0) : "memory");
	return d2;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register unsigned long d0 __asm__("d0") = call_id;
	register unsigned long d2 __asm__("d2");

	__asm__ volatile("syscall 0" : "=r"(d2) : "r"(d0) : "memory");
	return d2;
}

#ifdef CONFIG_USERSPACE

static inline bool arch_is_user_context(void)
{
	uint32_t psw;

	__asm__ volatile("	mfcr %0, 0xFE04\n\t" : "=r"(psw));

	if (((psw >> 10) & 0x3) == 0x2) {
		return false;
	}

	return true;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_SYSCALL_H_ */
