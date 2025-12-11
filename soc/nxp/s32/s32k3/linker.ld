/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Emit IVT section only when XIP and (standalone app OR MCUboot itself) */
#if defined(CONFIG_XIP) && (!defined(CONFIG_BOOTLOADER_MCUBOOT) || defined(CONFIG_MCUBOOT))

MEMORY
{
	IVT_HEADER (r) : ORIGIN = (CONFIG_FLASH_BASE_ADDRESS + CONFIG_IVT_HEADER_OFFSET),
			 LENGTH = CONFIG_IVT_HEADER_SIZE
}

/* Symbols for IVT */
PROVIDE(__ivt_region_start = ORIGIN(IVT_HEADER));
PROVIDE(__ivt_region_end   = ORIGIN(IVT_HEADER) + LENGTH(IVT_HEADER));

#endif

/* Include standard Zephyr ARM script */
#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
