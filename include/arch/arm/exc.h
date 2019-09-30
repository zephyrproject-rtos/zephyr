/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-M public exception handling
 *
 * ARM-specific kernel exception handling interface. Included by arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_EXC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_EXC_H_

/* for assembler, only works with constants */
#define Z_EXC_PRIO(pri) (((pri) << (8 - DT_NUM_IRQ_PRIO_BITS)) & 0xff)

#if defined(CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS)
#define _EXCEPTION_RESERVED_PRIO 1
#else
#define _EXCEPTION_RESERVED_PRIO 0
#endif

#define _EXC_FAULT_PRIO 0
#ifdef CONFIG_ZERO_LATENCY_IRQS
#define _EXC_ZERO_LATENCY_IRQS_PRIO 0
#define _EXC_SVC_PRIO 1
#define _IRQ_PRIO_OFFSET (_EXCEPTION_RESERVED_PRIO + 1)
#else
#define _EXC_SVC_PRIO 0
#define _IRQ_PRIO_OFFSET (_EXCEPTION_RESERVED_PRIO)
#endif

#define _EXC_IRQ_DEFAULT_PRIO Z_EXC_PRIO(_IRQ_PRIO_OFFSET)

#ifdef _ASMLANGUAGE
GTEXT(z_arm_exc_exit);
#else
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct __esf {
	struct __basic_sf {
		sys_define_gpr_with_alias(a1, r0);
		sys_define_gpr_with_alias(a2, r1);
		sys_define_gpr_with_alias(a3, r2);
		sys_define_gpr_with_alias(a4, r3);
		sys_define_gpr_with_alias(ip, r12);
		sys_define_gpr_with_alias(lr, r14);
		sys_define_gpr_with_alias(pc, r15);
		u32_t xpsr;
	} basic;
#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	float s[16];
	u32_t fpscr;
	u32_t undefined;
#endif
};

typedef struct __esf z_arch_esf_t;

extern void z_arm_exc_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_EXC_H_ */
