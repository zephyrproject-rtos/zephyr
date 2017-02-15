/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa public exception handling
 *
 * Xtensa-specific nanokernel exception handling interface. Included by
 * arch/xtensa/arch.h.
 */

#ifndef _ARCH_XTENSA_EXC_H_
#define _ARCH_XTENSA_EXC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
#else
/**
 * @brief Nanokernel Exception Stack Frame
 *
 * A pointer to an "exception stack frame" (ESF) is passed as an argument
 * to exception handlers registered via nanoCpuExcConnect().
 */
struct __esf {
	/* XXX - not finished yet */
	sys_define_gpr_with_alias(a1, sp);
	uint32_t pc;
};

typedef struct __esf NANO_ESF;
extern const NANO_ESF _default_esf;
#endif

#ifdef __cplusplus
}
#endif


#endif /* _ARCH_XTENSA_EXC_H_ */
