/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();
	__DSB();
	__ISB();
}
