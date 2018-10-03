/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cache helper functions and defines (ARC)
 *
 * This file contains cache related functions and definitions for the
 * ARCv2 processor architecture.
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_V2_CACHE_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_V2_CACHE_H_

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

/* i-cache defines for IC_CTRL register */
#define IC_CACHE_ENABLE   0x00
#define IC_CACHE_DISABLE  0x01
#define IC_CACHE_DIRECT   0x00
#define IC_CACHE_INDIRECT 0x20

/*
 * @brief Initialize the I-cache
 *
 * Enables the i-cache and sets it to direct access mode.
 */
static ALWAYS_INLINE void _icache_setup(void)
{
	u32_t icache_config = (
		IC_CACHE_DIRECT | /* direct mapping (one-way assoc.) */
		IC_CACHE_ENABLE   /* i-cache enabled */
	);
	u32_t val;

	val = _arc_v2_aux_reg_read(_ARC_V2_I_CACHE_BUILD);
	val &= 0xff;
	if (val != 0) { /* is i-cache present? */
		/* configure i-cache */
		_arc_v2_aux_reg_write(_ARC_V2_IC_CTRL, icache_config);
	}
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_V2_CACHE_H_ */
