/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>
#include <fsl_cache_lpcac.h>

void cache_instr_enable(void)
{
	L1CACHE_EnableCodeCache();
}

void cache_instr_disable(void)
{
	L1CACHE_DisableCodeCache();
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	L1CACHE_InvalidateCodeCache();

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	return -ENOTSUP;
}
