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
extern unsigned int z_arc_cpu_sleep_mode;

extern u32_t z_timer_cycle_get_32(void);

static inline u32_t z_arch_k_cycle_get_32(void)
{
	return z_timer_cycle_get_32();
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_MISC_H_ */
