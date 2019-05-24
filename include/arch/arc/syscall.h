/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARC specific sycall header
 *
 * This header contains the ARC specific sycall interface.  It is
 * included by the syscall interface architecture-abstraction header
 * (include/arch/syscall.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_SYSCALL_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_SYSCALL_H_

#define _TRAP_S_SCALL_IRQ_OFFLOAD		1
#define _TRAP_S_CALL_RUNTIME_EXCEPT		2
#define _TRAP_S_CALL_SYSTEM_CALL		3

#ifdef CONFIG_USERSPACE
#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

#ifdef CONFIG_CPU_ARCV2
#include <arch/arc/v2/aux_regs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* Syscall invocation macros. arc-specific machine constraints used to ensure
 * args land in the proper registers. Currently, they are all stub functions
 * just for enabling CONFIG_USERSPACE on arc w/o errors.
 */

static inline u32_t z_arch_syscall_invoke6(u32_t arg1, u32_t arg2, u32_t arg3,
					  u32_t arg4, u32_t arg5, u32_t arg6,
					  u32_t call_id)
{
	register u32_t ret __asm__("r0") = arg1;
	register u32_t r1 __asm__("r1") = arg2;
	register u32_t r2 __asm__("r2") = arg3;
	register u32_t r3 __asm__("r3") = arg4;
	register u32_t r4 __asm__("r4") = arg5;
	register u32_t r5 __asm__("r5") = arg6;
	register u32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r5), "r" (r6));

	return ret;
}

static inline u32_t z_arch_syscall_invoke5(u32_t arg1, u32_t arg2, u32_t arg3,
					  u32_t arg4, u32_t arg5, u32_t call_id)
{
	register u32_t ret __asm__("r0") = arg1;
	register u32_t r1 __asm__("r1") = arg2;
	register u32_t r2 __asm__("r2") = arg3;
	register u32_t r3 __asm__("r3") = arg4;
	register u32_t r4 __asm__("r4") = arg5;
	register u32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r6));

	return ret;
}

static inline u32_t z_arch_syscall_invoke4(u32_t arg1, u32_t arg2, u32_t arg3,
					  u32_t arg4, u32_t call_id)
{
	register u32_t ret __asm__("r0") = arg1;
	register u32_t r1 __asm__("r1") = arg2;
	register u32_t r2 __asm__("r2") = arg3;
	register u32_t r3 __asm__("r3") = arg4;
	register u32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r6));

	return ret;
}

static inline u32_t z_arch_syscall_invoke3(u32_t arg1, u32_t arg2, u32_t arg3,
					  u32_t call_id)
{
	register u32_t ret __asm__("r0") = arg1;
	register u32_t r1 __asm__("r1") = arg2;
	register u32_t r2 __asm__("r2") = arg3;
	register u32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r6));

	return ret;
}

static inline u32_t z_arch_syscall_invoke2(u32_t arg1, u32_t arg2, u32_t call_id)
{
	register u32_t ret __asm__("r0") = arg1;
	register u32_t r1 __asm__("r1") = arg2;
	register u32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r1), "r" (r6));

	return ret;
}

static inline u32_t z_arch_syscall_invoke1(u32_t arg1, u32_t call_id)
{
	register u32_t ret __asm__("r0") = arg1;
	register u32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r6));

	return ret;
}

static inline u32_t z_arch_syscall_invoke0(u32_t call_id)
{
	register u32_t ret __asm__("r0");
	register u32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "trap_s %[trap_s_id]\n"
			 : "=r"(ret)
			 : [trap_s_id] "i" (_TRAP_S_CALL_SYSTEM_CALL),
			   "r" (ret), "r" (r6));

	return ret;
}

static inline bool z_arch_is_user_context(void)
{
	u32_t status;

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
