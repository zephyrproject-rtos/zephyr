/* cache.h - cache helper functions (ARC) */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
