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

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Additional register state that is not stacked by hardware on exception
 * entry.
 *
 * The fault paths fill these fields in the exception frame before calling
 * z_arm_fatal_error(). They are not valid in other exception frames. msp
 * and exc_return are always zero on Cortex-A/R.
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
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	struct __extra_esf_info extra_info;
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

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_A_R_EXCEPTION_H_ */
