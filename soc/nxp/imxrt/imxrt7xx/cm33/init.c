/*
 * Copyright 2024,2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for NXP RT7XX platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the RT7XX platforms.
 */

#include <zephyr/init.h>
#include <zephyr/autoconf.h>
#include <zephyr/cache.h>
#include <soc.h>


#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
#include <zephyr_image_info.h>
/* Memcpy macro to copy segments from secondary core image stored in flash
 * to RAM section that secondary core boots from.
 * n is the segment number, as defined in zephyr_image_info.h
 */
#define MEMCPY_SEGMENT(n, _)                                                                       \
	memcpy((uint32_t *)((SEGMENT_LMA_ADDRESS_##n) - ADJUSTED_LMA),                             \
	       (uint32_t *)(SEGMENT_LMA_ADDRESS_##n), (SEGMENT_SIZE_##n))
#endif

void soc_early_init_hook(void)
{
#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
	/**
	 * Copy CM33 CPU1 core from flash to memory. Note that depending on where the
	 * user decided to store CPU1 code, this is likely going to read from the
	 * XSPI while using XIP. Provided we DO NOT WRITE TO THE XSPI,
	 * this operation is safe.
	 *
	 * Note that this copy MUST occur before enabling the CPU0 caching to
	 * ensure the data is written directly to RAM (since the CPU1 core will use it)
	 */
	LISTIFY(SEGMENT_NUM, MEMCPY_SEGMENT, (;));
#endif

	/* Enable data cache */
	sys_cache_data_enable();

	__ISB();
	__DSB();
}

void soc_reset_hook(void)
{
	SystemInit();
}
