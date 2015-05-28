/* memory_map.h - ARM CORTEX-M memory map */

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
DESCRIPTION
This module contains definitions for the memory map of the CORTEX-M series of
processors.

*/

#ifndef _CORTEXM_MEMORY_MAP__H_
#define _CORTEXM_MEMORY_MAP__H_

#include <misc/util.h>

/* 0x00000000 -> 0x1fffffff: Code in ROM [0.5 GB] */
#define _CODE_BASE_ADDR 0
#define _CODE_END_ADDR (_CODE_BASE_ADDR + MB(512) - 1)

/* 0x20000000 -> 0x3fffffff: SRAM [0.5GB] */
#define _SRAM_BASE_ADDR (_CODE_END_ADDR + 1)
#define _SRAM_BIT_BAND_REGION (_SRAM_BASE_ADDR)
#define _SRAM_BIT_BAND_REGION_END (_SRAM_BIT_BAND_REGION + MB(1) - 1)
#define _SRAM_BIT_BAND_ALIAS (_SRAM_BIT_BAND_REGION + MB(32))
#define _SRAM_BIT_BAND_ALIAS_END (_SRAM_BIT_BAND_ALIAS + MB(32) - 1)
#define _SRAM_END_ADDR (_SRAM_BASE_ADDR + MB(512) - 1)

/* 0x40000000 -> 0x5fffffff: Peripherals [0.5GB] */
#define _PERI_BASE_ADDR (_SRAM_END_ADDR + 1)
#define _PERI_BIT_BAND_REGION (_PERI_BASE_ADDR)
#define _PERI_BIT_BAND_REGION_END (_PERI_BIT_BAND_REGION + MB(1) - 1)
#define _PERI_BIT_BAND_ALIAS (_PERI_BIT_BAND_REGION + MB(32))
#define _PERI_BIT_BAND_ALIAS_END (_PERI_BIT_BAND_ALIAS + MB(32) - 1)
#define _PERI_END_ADDR (_PERI_BASE_ADDR + MB(512) - 1)

/* 0x60000000 -> 0x9fffffff: external RAM [1GB] */
#define _ERAM_BASE_ADDR (_PERI_END_ADDR + 1)
#define _ERAM_END_ADDR (_ERAM_BASE_ADDR + GB(1) - 1)

/* 0xa0000000 -> 0xdfffffff: external devices [1GB] */
#define _EDEV_BASE_ADDR (_ERAM_END_ADDR + 1)
#define _EDEV_END_ADDR (_EDEV_BASE_ADDR + GB(1) - 1)

/* 0xe0000000 -> 0xffffffff is different between M3 and M0 */

#if defined(CONFIG_CPU_CORTEXM3)
#include <arch/arm/CortexM/memory_map-m3.h>
#elif defined(CONFIG_CPU_CORTEXM0)
#include <arch/arm/CortexM/memory_map-m0.h>
#else
#error Unknown CPU
#endif

#endif /* _CORTEXM_MEMORY_MAP__H_ */
