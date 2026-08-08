/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/init.h>

static int rcar_enable_cache(void)
{
	sys_cache_instr_flush_all();
	sys_cache_instr_enable();
	sys_cache_data_invd_all();
	sys_cache_data_enable();
	__DSB();
	__ISB();

	return 0;
}

SYS_INIT(rcar_enable_cache, PRE_KERNEL_2, 0);
