/*
 * Copyright (c) 2025 Alif Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>

void soc_reset_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();
}
