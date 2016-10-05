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
 * @brief ARM CORTEX-M memory map
 *
 * This module contains definitions for the memory map of the CORTEX-M series of
 * processors.
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

#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS) || defined(CONFIG_CPU_CORTEX_M3_M4)
/* 0xe0000000 -> 0xe00fffff: private peripheral bus */

/* 0xe0000000 -> 0xe003ffff: internal [256KB] */
#define _PPB_INT_BASE_ADDR (_EDEV_END_ADDR + 1)
#define _PPB_INT_ITM _PPB_INT_BASE_ADDR
#define _PPB_INT_DWT (_PPB_INT_ITM + KB(4))
#define _PPB_INT_FPB (_PPB_INT_DWT + KB(4))
#define _PPB_INT_RSVD_1 (_PPB_INT_FPB + KB(4))
#define _PPB_INT_SCS (_PPB_INT_RSVD_1 + KB(44))
#define _PPB_INT_RSVD_2 (_PPB_INT_SCS + KB(4))
#define _PPB_INT_END_ADDR (_PPB_INT_RSVD_2 + KB(196) - 1)

/* 0xe0040000 -> 0xe00fffff: external [768K] */
#define _PPB_EXT_BASE_ADDR (_PPB_INT_END_ADDR + 1)
#define _PPB_EXT_TPIU _PPB_EXT_BASE_ADDR
#define _PPB_EXT_ETM (_PPB_EXT_TPIU + KB(4))
#define _PPB_EXT_PPB (_PPB_EXT_ETM + KB(4))
#define _PPB_EXT_ROM_TABLE (_PPB_EXT_PPB + KB(756))
#define _PPB_EXT_END_ADDR (_PPB_EXT_ROM_TABLE + KB(4) - 1)

/* 0xe0100000 -> 0xffffffff: vendor-specific [0.5GB-1MB or 511MB]  */
#define _VENDOR_BASE_ADDR (_PPB_EXT_END_ADDR + 1)
#define _VENDOR_END_ADDR 0xffffffff
#else
#error Unknown CPU
#endif

#endif /* _CORTEXM_MEMORY_MAP__H_ */
