/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Cache helper functions and defines (ARC)
 *
 * This file contains cache related functions and definitions for the
 * ARCv2 processor architecture.
 */

#ifndef _ARCV2_CACHE__H_
#define _ARCV2_CACHE__H_

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
	uint32_t icache_config = (
		IC_CACHE_DIRECT | /* direct mapping (one-way assoc.) */
		IC_CACHE_ENABLE   /* i-cache enabled */
	);
	uint32_t val;

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

#endif /* _ARCV2_CACHE__H_ */
