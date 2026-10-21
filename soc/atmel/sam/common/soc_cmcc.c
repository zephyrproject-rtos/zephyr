/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/sys/util_macro.h>
#include <soc.h>

void soc_cmcc_init(void)
{
	if (IS_ENABLED(CONFIG_CACHE_MANAGEMENT) && IS_ENABLED(CONFIG_ICACHE)) {
		sys_cache_instr_enable();
	} else {
		/* A bootloader can leave the cache enabled, disable it to stay flash coherent */
		CMCC->CMCC_CTRL = 0U;
	}
}
