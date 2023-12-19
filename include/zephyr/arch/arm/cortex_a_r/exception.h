/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 Cortex-A and Cortex-R public exception handling
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_EXCEPTION_H_

#ifdef _ASMLANGUAGE
GTEXT(z_arm_exc_exit);
#else
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)

/* Registers s16-s31 (d8-d15, q4-q7) must be preserved across subroutine calls.
 *
 * Registers s0-s15 (d0-d7, q0-q3) do not have to be preserved (and can be used
 * for passing arguments or returning results in standard procedure-call variants).
 *
 * Registers d16-d31 (q8-q15), do not have to be preserved.
 */
struct __fpu_sf {
	uint32_t s[16]; /* s0~s15 (d0-d7) */
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
	uint64_t d[16]; /* d16~d31 */
#endif
	uint32_t fpscr;
	uint32_t undefined;
};
#endif

/* Additional register state that is not stacked by hardware on exception
 * entry.
 *
 * These fields are ONLY valid in the ESF copy passed into z_arm_fatal_error().
 * When information for a member is unavailable, the field is set to zero.
 */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
struct __extra_esf_info {
	_callee_saved_t *callee;
	uint32_t msp;
	uint32_t exc_return;
};
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */

struct __esf {
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	struct __extra_esf_info extra_info;
#endif
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	struct __fpu_sf fpu;
#endif
	struct __basic_sf {
		sys_define_gpr_with_alias(a1, r0);
		sys_define_gpr_with_alias(a2, r1);
		sys_define_gpr_with_alias(a3, r2);
		sys_define_gpr_with_alias(a4, r3);
		sys_define_gpr_with_alias(ip, r12);
		sys_define_gpr_with_alias(lr, r14);
		sys_define_gpr_with_alias(pc, r15);
		uint32_t xpsr;
	} basic;
};

extern uint32_t z_arm_coredump_fault_sp;

typedef struct __esf z_arch_esf_t;

extern void z_arm_exc_exit(bool fatal);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_EXCEPTION_H_ */
