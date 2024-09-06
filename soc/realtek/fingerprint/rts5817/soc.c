/*
 * Copyright (c) 2025 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <soc.h>

#include <cmsis_core.h>

/**
 * @brief Initialize cache
 *
 */
static void cache_init(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	cache_init();
}
