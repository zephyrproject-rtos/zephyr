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

#include <zephyr/devicetree.h>

#include <zephyr/arch/arm/cortex_m/nvic.h>

/* for assembler, only works with constants */
#define Z_EXC_PRIO(pri) (((pri) << (8 - NUM_IRQ_PRIO_BITS)) & 0xff)

/*
 * In architecture variants with non-programmable fault exceptions
 * (e.g. Cortex-M Baseline variants), hardware ensures processor faults
 * are given the highest interrupt priority level. SVCalls are assigned
 * the highest configurable priority level (level 0); note, however, that
 * this interrupt level may be shared with HW interrupts.
 *
 * In Cortex variants with programmable fault exception priorities we
 * assign the highest interrupt priority level (level 0) to processor faults
 * with configurable priority.
 * The highest priority level may be shared with either Zero-Latency IRQs (if
 * support for the feature is enabled) or with SVCall priority level.
 * Regular HW IRQs are always assigned priority levels lower than the priority
 * levels for SVCalls, Zero-Latency IRQs and processor faults.
 *
 * PendSV IRQ (which is used in Cortex-M variants to implement thread
 * context-switching) is assigned the lowest IRQ priority level.
 */
#if defined(CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS)
#define _EXCEPTION_RESERVED_PRIO 1
#else
#define _EXCEPTION_RESERVED_PRIO 0
#endif

#define _EXC_FAULT_PRIO 0
#define _EXC_ZERO_LATENCY_IRQS_PRIO 0
#define _EXC_SVC_PRIO COND_CODE_1(CONFIG_ZERO_LATENCY_IRQS,		\
				  (CONFIG_ZERO_LATENCY_LEVELS), (0))
#define _IRQ_PRIO_OFFSET (_EXCEPTION_RESERVED_PRIO + _EXC_SVC_PRIO)
#define IRQ_PRIO_LOWEST (BIT(NUM_IRQ_PRIO_BITS) - (_IRQ_PRIO_OFFSET) - 1)

#define _EXC_IRQ_DEFAULT_PRIO Z_EXC_PRIO(_IRQ_PRIO_OFFSET)

/* Use lowest possible priority level for PendSV */
#define _EXC_PENDSV_PRIO 0xff
#define _EXC_PENDSV_PRIO_MASK Z_EXC_PRIO(_EXC_PENDSV_PRIO)

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

/* ARM GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { uint32_t name1, name2; }

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

extern void z_arm_exc_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_EXCEPTION_H_ */
