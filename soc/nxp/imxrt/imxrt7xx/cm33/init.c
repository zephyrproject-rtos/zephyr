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

void soc_early_init_hook(void)
{
	/* Enable data cache */
	sys_cache_data_enable();

	__ISB();
	__DSB();
}

void soc_reset_hook(void)
{
	SystemInit();
}
