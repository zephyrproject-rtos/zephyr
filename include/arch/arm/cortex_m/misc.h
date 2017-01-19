/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-M public kernel miscellaneous
 *
 * ARM-specific kernel miscellaneous interface. Included by arm/arch.h.
 */

#ifndef _ARCH_ARM_CORTEXM_MISC_H_
#define _ARCH_ARM_CORTEXM_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern void k_cpu_idle(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_ARM_CORTEXM_MISC_H_ */
