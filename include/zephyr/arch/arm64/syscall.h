/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 specific syscall header
 *
 * This header contains the ARM64 specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch64/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_SYSCALL_H_

#define _SVC_CALL_IRQ_OFFLOAD		1
#define _SVC_CALL_RUNTIME_EXCEPT	2
#define _SVC_CALL_SYSTEM_CALL		3

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/arch/arm64/tpidrro_el0.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Syscall invocation macros. arm-specific machine constraints used to ensure
 * args land in the proper registers.
 */
static inline uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id)
{
	register uint64_t ret __asm__("x0") = arg1;
	register uint64_t r1 __asm__("x1") = arg2;
	register uint64_t r2 __asm__("x2") = arg3;
	register uint64_t r3 __asm__("x3") = arg4;
	register uint64_t r4 __asm__("x4") = arg5;
	register uint64_t r5 __asm__("x5") = arg6;
	register uint64_t r8 __asm__("x8") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r5), "r" (r8)
			 : "memory");

	return ret;
}

static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5,
					     uintptr_t call_id)
{
	register uint64_t ret __asm__("x0") = arg1;
	register uint64_t r1 __asm__("x1") = arg2;
	register uint64_t r2 __asm__("x2") = arg3;
	register uint64_t r3 __asm__("x3") = arg4;
	register uint64_t r4 __asm__("x4") = arg5;
	register uint64_t r8 __asm__("x8") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r8)
			 : "memory");

	return ret;
}

static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id)
{
	register uint64_t ret __asm__("x0") = arg1;
	register uint64_t r1 __asm__("x1") = arg2;
	register uint64_t r2 __asm__("x2") = arg3;
	register uint64_t r3 __asm__("x3") = arg4;
	register uint64_t r8 __asm__("x8") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r8)
			 : "memory");

	return ret;
}

static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3,
					     uintptr_t call_id)
{
	register uint64_t ret __asm__("x0") = arg1;
	register uint64_t r1 __asm__("x1") = arg2;
	register uint64_t r2 __asm__("x2") = arg3;
	register uint64_t r8 __asm__("x8") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r8)
			 : "memory");

	return ret;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)
{
	register uint64_t ret __asm__("x0") = arg1;
	register uint64_t r1 __asm__("x1") = arg2;
	register uint64_t r8 __asm__("x8") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r8)
			 : "memory");

	return ret;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1,
					     uintptr_t call_id)
{
	register uint64_t ret __asm__("x0") = arg1;
	register uint64_t r8 __asm__("x8") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r8)
			 : "memory");
	return ret;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register uint64_t ret __asm__("x0");
	register uint64_t r8 __asm__("x8") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r8)
			 : "memory");

	return ret;
}

static inline bool arch_is_user_context(void)
{
	return (read_tpidrro_el0() & TPIDRROEL0_IN_EL0) != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_SYSCALL_H_ */
