/* memory_map.h - ARM CORTEX-M memory map */

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

/* 0xe0000000 -> 0xffffffff: varies by processor (see below) */

#if defined(CONFIG_CPU_CORTEX_M3_M4)
#include <arch/arm/cortex_m/memory_map-m3-m4.h>
#else
#error Unknown CPU
#endif

#endif /* _CORTEXM_MEMORY_MAP__H_ */
