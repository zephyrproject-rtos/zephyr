/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 Cortex-M public exception handling
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_EXCEPTION_H_

/*
 * The exception/interrupt priority constants (_EXC_IRQ_DEFAULT_PRIO, etc.) live
 * in the lightweight cpu.h so that low-level headers can use them without
 * pulling in the Cortex-M context-switch machinery (and <kernel/thread.h>).
 */
#include <zephyr/arch/arm/cortex_m/cpu.h>

#ifdef _ASMLANGUAGE
#ifndef CONFIG_USE_SWITCH
GTEXT(z_arm_exc_exit);
#endif
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
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* System control registers inspected and/or modified during fault handling */
	uint32_t cfsr;
	uint32_t hfsr;
	uint32_t dfsr;
	uint32_t mmfar;
	uint32_t bfar;
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	uint32_t sfsr;
	uint32_t sfar;
	/* Exception entry occurred in Non-Secure stack */
	bool non_secure;
#endif /* CONFIG_ARM_SECURE_FIRMWARE*/
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
};
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */

/* ARM GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2)                                                    \
	union {                                                                                    \
		uint32_t name1, name2;                                                             \
	}

struct arch_esf {
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
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	struct __fpu_sf fpu;
#endif
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	struct __extra_esf_info extra_info;
#endif
};

extern uint32_t z_arm_coredump_fault_sp;

#ifndef CONFIG_USE_SWITCH
extern void z_arm_exc_exit(void);
#endif
/*
 * For CONFIG_USE_SWITCH, z_arm_exc_exit() is provided as an inline wrapper
 * around arm_m_exc_tail() by <zephyr/arch/arm/arm-m-switch.h>, so that this
 * exception header does not have to include the switch machinery (and
 * <kernel/thread.h>).
 */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_EXCEPTION_H_ */
