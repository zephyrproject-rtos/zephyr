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

#ifndef _ARCH_ARM_CORTEXM_EXC_H_
#define _ARCH_ARM_CORTEXM_EXC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(_ExcExit);
#else
#include <stdint.h>

struct __esf {
	sys_define_gpr_with_alias(a1, r0);
	sys_define_gpr_with_alias(a2, r1);
	sys_define_gpr_with_alias(a3, r2);
	sys_define_gpr_with_alias(a4, r3);
	sys_define_gpr_with_alias(ip, r12);
	sys_define_gpr_with_alias(lr, r14);
	sys_define_gpr_with_alias(pc, r15);
	uint32_t xpsr;
#ifdef CONFIG_FLOAT
	float s[16];
	uint32_t fpscr;
	uint32_t undefined;
#endif
};

typedef struct __esf NANO_ESF;

extern const NANO_ESF _default_esf;

extern void _ExcExit(void);

/**
 * @brief display the contents of a exception stack frame
 *
 * @return N/A
 */

extern void sys_exc_esf_dump(NANO_ESF *esf);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_ARM_CORTEXM_EXC_H_ */
