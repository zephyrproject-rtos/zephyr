/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <soc.h>

/*******************************************************************************
 * Re-implementation/Overrides __((weak)) symbol functions from ethosu_driver.c
 *******************************************************************************/

#ifndef CONFIG_NOCACHE_MEMORY

void ethosu_flush_dcache(uint32_t *p, size_t bytes)
{
	if (p && bytes) {
		SCB_CleanDCache_by_Addr((void *)p, (int32_t)bytes);
	}
}

void ethosu_invalidate_dcache(uint32_t *p, size_t bytes)
{
	if (p && bytes) {
		SCB_InvalidateDCache_by_Addr((void *)p, (int32_t)bytes);
	}
}

#endif
