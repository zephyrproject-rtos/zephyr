/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 public kernel miscellaneous
 *
 * ARC-specific kernel miscellaneous interface. Included by arc/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_MISC_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern unsigned int k_cpu_sleep_mode;
extern void k_cpu_idle(void);
extern void k_cpu_atomic_idle(unsigned int key);

extern u32_t _timer_cycle_get_32(void);
#define _arch_k_cycle_get_32()	_timer_cycle_get_32()
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_MISC_H_ */
