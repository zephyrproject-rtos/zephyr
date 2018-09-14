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

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MISC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern void k_cpu_idle(void);

extern u32_t _timer_cycle_get_32(void);
#define _arch_k_cycle_get_32()	_timer_cycle_get_32()
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MISC_H_ */
