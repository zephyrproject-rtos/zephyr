/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_XIP
MEMORY
{
	IVT_HEADER (r) : ORIGIN = CONFIG_FLASH_BASE_ADDRESS + CONFIG_IVT_HEADER_OFFSET,
			 LENGTH = CONFIG_IVT_HEADER_SIZE
}
#endif

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
