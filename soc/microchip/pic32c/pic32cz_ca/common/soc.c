/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/init.h>

#include <soc.h>

static void soc_flash_read_init(void)
{
	FCR_REGS->FCR_CTRLA |= FCR_CTRLA_ADRWS_Msk;
}

void soc_early_init_hook(void)
{
	soc_flash_read_init();

	sys_cache_instr_enable();
	sys_cache_data_enable();
}
