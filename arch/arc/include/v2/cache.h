/* cache.h - cache helper functions (ARC) */

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

/*
 * DESCRIPTION
 * This file contains private nanokernel structures definitions and various
 * other definitions for the ARCv2 processor architecture.
 */

#ifndef _ARCV2_CACHE__H_
#define _ARCV2_CACHE__H_

#include <arch/cpu.h>

#ifndef _ASMLANGUAGE

#define CACHE_ENABLE 0x00
#define CACHE_DISABLE 0x01
#define CACHE_DIRECT 0x00
#define CACHE_CACHE_CONTROLLED 0x20

/*
 * @brief Sets the I-cache
 *
 * Enables cache and sets the direct access.
 */
static ALWAYS_INLINE void _icache_setup(void)
{
	uint32_t icache_config = (
		CACHE_DIRECT | /* direct mapping (one-way assoc.) */
		CACHE_ENABLE   /* i-cache enabled */
	);
	_arc_v2_aux_reg_write(_ARC_V2_IC_CTRL, icache_config);
}

#endif /* _ASMLANGUAGE */
#endif /* _ARCV2_CACHE__H_ */
