/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>

#include <OsIf.h>

extern void swt_disable(void);

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

	swt_disable();
	OsIf_Init(NULL);
}
