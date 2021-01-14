/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 specific syscall header
 *
 * This header contains the ARM AArch32 specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_SYSCALL_H_

#define _SVC_CALL_CONTEXT_SWITCH	0
#define _SVC_CALL_IRQ_OFFLOAD		1
#define _SVC_CALL_RUNTIME_EXCEPT	2
#define _SVC_CALL_SYSTEM_CALL		3

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Syscall invocation macros. arm-specific machine constraints used to ensure
 * args land in the proper registers.
 */
static inline uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5, uintptr_t arg6,
					     uintptr_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r2 __asm__("r2") = arg3;
	register uint32_t r3 __asm__("r3") = arg4;
	register uint32_t r4 __asm__("r4") = arg5;
	register uint32_t r5 __asm__("r5") = arg6;
	register uint32_t r6 __asm__("r6") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret), "=r"(r1), "=r"(r2), "=r"(r3)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r5), "r" (r6)
			 : "r8", "memory", "ip");

	return ret;
}

static inline uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t arg5,
					     uintptr_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r2 __asm__("r2") = arg3;
	register uint32_t r3 __asm__("r3") = arg4;
	register uint32_t r4 __asm__("r4") = arg5;
	register uint32_t r6 __asm__("r6") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret), "=r"(r1), "=r"(r2), "=r"(r3)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r6)
			 : "r8", "memory", "ip");

	return ret;
}

static inline uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3, uintptr_t arg4,
					     uintptr_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r2 __asm__("r2") = arg3;
	register uint32_t r3 __asm__("r3") = arg4;
	register uint32_t r6 __asm__("r6") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret), "=r"(r1), "=r"(r2), "=r"(r3)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r6)
			 : "r8", "memory", "ip");

	return ret;
}

static inline uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t arg3,
					     uintptr_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r2 __asm__("r2") = arg3;
	register uint32_t r6 __asm__("r6") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret), "=r"(r1), "=r"(r2)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r6)
			 : "r8", "memory", "r3", "ip");

	return ret;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r6 __asm__("r6") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret), "=r"(r1)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r6)
			 : "r8", "memory", "r2", "r3", "ip");

	return ret;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1,
					     uintptr_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r6 __asm__("r6") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r6)
			 : "r8", "memory", "r1", "r2", "r3", "ip");
	return ret;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register uint32_t ret __asm__("r0");
	register uint32_t r6 __asm__("r6") = call_id;

	__asm__ volatile("svc %[svid]\n"
			 : "=r"(ret)
			 : [svid] "i" (_SVC_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r6)
			 : "r8", "memory", "r1", "r2", "r3", "ip");

	return ret;
}

static inline bool arch_is_user_context(void)
{
	uint32_t value;

	/* check for handler mode */
	__asm__ volatile("mrs %0, IPSR\n\t" : "=r"(value));
	if (value) {
		return false;
	}

	/* if not handler mode, return mode information */
	__asm__ volatile("mrs %0, CONTROL\n\t" : "=r"(value));
	return (value & CONTROL_nPRIV_Msk) ? true : false;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_SYSCALL_H_ */
