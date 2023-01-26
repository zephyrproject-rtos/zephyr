/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARC specific syscall header
 *
 * This header contains the ARC specific syscall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_SYSCALL_H_

#define _TRAP_S_CALL_RUNTIME_EXCEPT		2
#define _TRAP_S_CALL_SYSTEM_CALL		3

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

#ifdef CONFIG_ISA_ARCV2
#include <zephyr/arch/arc/v2/aux_regs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* Syscall invocation macros. arc-specific machine constraints used to ensure
 * args land in the proper registers. Currently, they are all stub functions
 * just for enabling CONFIG_USERSPACE on arc w/o errors.
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

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r5), "r" (r6));

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

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r6));

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

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r6));

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

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r6));

	return ret;
}

static inline uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
					     uintptr_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r6));

	return ret;
}

static inline uintptr_t arch_syscall_invoke1(uintptr_t arg1, uintptr_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r6));

	return ret;
}

static inline uintptr_t arch_syscall_invoke0(uintptr_t call_id)
{
	register uint32_t ret __asm__("r0");
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r6));

	return ret;
}

static inline bool arch_is_user_context(void)
{
	uint32_t status;

	compiler_barrier();

	__asm__ volatile("lr %0, [%[status32]]\n"
			 : "=r"(status)
			 : [status32] "i" (_ARC_V2_STATUS32));

	return !(status & _ARC_V2_STATUS32_US) ? true : false;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARC_SYSCALL_H_ */
