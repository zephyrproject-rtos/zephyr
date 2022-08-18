/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 */

#include <zephyr/linker/linker-tool.h>

#define ROM_VMA_ADDR (CONFIG_FLASH_VMA_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)

#if CONFIG_FLASH_LOAD_SIZE > 0
	#define ROM_VMA_SIZE CONFIG_FLASH_LOAD_SIZE
#else
	#define ROM_VMA_SIZE (CONFIG_FLASH_SIZE*1K - CONFIG_FLASH_LOAD_OFFSET)
#endif

MEMORY
     {
	 FLASH_VMA  (rx) : ORIGIN = ROM_VMA_ADDR, LENGTH = ROM_VMA_SIZE
     }

#if defined(CONFIG_XIP)
#undef GROUP_LINK_IN
#define GROUP_LINK_IN(where) > FLASH_VMA AT > where
#undef GROUP_ROM_LINK_IN
#define GROUP_ROM_LINK_IN(vregion, lregion) > FLASH_VMA AT > lregion
#endif

#include <zephyr/arch/arm/aarch32/cortex_m/scripts/linker.ld>
