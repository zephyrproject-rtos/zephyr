/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM CORTEX-M memory map
 *
 * This module contains definitions for the memory map of the CORTEX-M series of
 * processors.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MEMORY_MAP_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MEMORY_MAP_H_

#include <zephyr/sys/util.h>

/* 0x00000000 -> 0x1fffffff: Code in ROM [0.5 GB] */
#define _CODE_BASE_ADDR           0x00000000
#define _CODE_END_ADDR            0x1FFFFFFF

/* 0x20000000 -> 0x3fffffff: SRAM [0.5GB] */
#define _SRAM_BASE_ADDR           0x20000000
#define _SRAM_BIT_BAND_REGION     0x20000000
#define _SRAM_BIT_BAND_REGION_END 0x200FFFFF
#define _SRAM_BIT_BAND_ALIAS      0x22000000
#define _SRAM_BIT_BAND_ALIAS_END  0x23FFFFFF
#define _SRAM_END_ADDR            0x3FFFFFFF

/* 0x40000000 -> 0x5fffffff: Peripherals [0.5GB] */
#define _PERI_BASE_ADDR           0x40000000
#define _PERI_BIT_BAND_REGION     0x40000000
#define _PERI_BIT_BAND_REGION_END 0x400FFFFF
#define _PERI_BIT_BAND_ALIAS      0x42000000
#define _PERI_BIT_BAND_ALIAS_END  0x43FFFFFF
#define _PERI_END_ADDR            0x5FFFFFFF

/* 0x60000000 -> 0x9fffffff: external RAM [1GB] */
#define _ERAM_BASE_ADDR           0x60000000
#define _ERAM_END_ADDR            0x9FFFFFFF

/* 0xa0000000 -> 0xdfffffff: external devices [1GB] */
#define _EDEV_BASE_ADDR           0xA0000000
#define _EDEV_END_ADDR            0xDFFFFFFF

/* 0xe0000000 -> 0xffffffff: varies by processor (see below) */

/* 0xe0000000 -> 0xe00fffff: private peripheral bus */
/* 0xe0000000 -> 0xe003ffff: internal [256KB] */
#define _PPB_INT_BASE_ADDR        0xE0000000
#if defined(CONFIG_CPU_CORTEX_M0) || defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M1)
#define _PPB_INT_RSVD_0           0xE0000000
#define _PPB_INT_DWT              0xE0001000
#define _PPB_INT_BPU              0xE0002000
#define _PPB_INT_RSVD_1           0xE0003000
#define _PPB_INT_SCS              0xE000E000
#define _PPB_INT_RSVD_2           0xE000F000
#elif defined(CONFIG_CPU_CORTEX_M3) || defined(CONFIG_CPU_CORTEX_M4) || defined(CONFIG_CPU_CORTEX_M7)
#define _PPB_INT_ITM              0xE0000000
#define _PPB_INT_DWT              0xE0001000
#define _PPB_INT_FPB              0xE0002000
#define _PPB_INT_RSVD_1           0xE0003000
#define _PPB_INT_SCS              0xE000E000
#define _PPB_INT_RSVD_2           0xE000F000
#elif defined(CONFIG_CPU_CORTEX_M23) || \
	defined(CONFIG_CPU_CORTEX_M33) || \
	defined(CONFIG_CPU_CORTEX_M55) || \
	defined(CONFIG_CPU_CORTEX_M85)
#define _PPB_INT_RSVD_0           0xE0000000
#define _PPB_INT_SCS              0xE000E000
#define _PPB_INT_SCB              0xE000ED00
#define _PPB_INT_RSVD_1           0xE002E000
#else
#error Unknown CPU
#endif
#define _PPB_INT_END_ADDR         0xE003FFFF

/* 0xe0000000 -> 0xe00fffff: private peripheral bus */
/* 0xe0040000 -> 0xe00fffff: external [768K] */
#define _PPB_EXT_BASE_ADDR        0xE0040000
#if defined(CONFIG_CPU_CORTEX_M0) || defined(CONFIG_CPU_CORTEX_M0PLUS) \
	|| defined(CONFIG_CPU_CORTEX_M1) || defined(CONFIG_CPU_CORTEX_M23)
#elif defined(CONFIG_CPU_CORTEX_M3) || defined(CONFIG_CPU_CORTEX_M4)
#define _PPB_EXT_TPIU             0xE0040000
#define _PPB_EXT_ETM              0xE0041000
#define _PPB_EXT_PPB              0xE0042000
#define _PPB_EXT_ROM_TABLE        0xE00FF000
#define _PPB_EXT_END_ADDR         0xE00FFFFF
#elif defined(CONFIG_CPU_CORTEX_M33) || defined(CONFIG_CPU_CORTEX_M55) \
	|| defined(CONFIG_CPU_CORTEX_M85)
#undef  _PPB_EXT_BASE_ADDR
#define _PPB_EXT_BASE_ADDR        0xE0044000
#define _PPB_EXT_ROM_TABLE        0xE00FF000
#define _PPB_EXT_END_ADDR         0xE00FFFFF
#elif defined(CONFIG_CPU_CORTEX_M7)
#define _PPB_EXT_BASE_ADDR        0xE0040000
#define _PPB_EXT_RSVD_TPIU        0xE0040000
#define _PPB_EXT_ETM              0xE0041000
#define _PPB_EXT_CTI              0xE0042000
#define _PPB_EXT_PPB              0xE0043000
#define _PPB_EXT_PROC_ROM_TABLE   0xE00FE000
#define _PPB_EXT_PPB_ROM_TABLE    0xE00FF000
#else
#error Unknown CPU
#endif
#define _PPB_EXT_END_ADDR         0xE00FFFFF

/* 0xe0100000 -> 0xffffffff: vendor-specific [0.5GB-1MB or 511MB]  */
#define _VENDOR_BASE_ADDR         0xE0100000
#define _VENDOR_END_ADDR          0xFFFFFFFF

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MEMORY_MAP_H_ */
