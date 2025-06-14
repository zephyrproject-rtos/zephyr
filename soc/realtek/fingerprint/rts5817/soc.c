/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
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
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int realtek_rts5817_init(void)
{
	cache_init();

	return 0;
}

SYS_INIT(realtek_rts5817_init, PRE_KERNEL_1, 0);
