/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include "fsl_cache.h"

void cache_data_enable(void)
{
	L1CACHE_EnableSystemCache();
}

void cache_data_disable(void)
{
	L1CACHE_DisableSystemCache();
}

void cache_instr_enable(void)
{
	L1CACHE_EnableCodeCache();
}

void cache_instr_disable(void)
{
	L1CACHE_DisableCodeCache();
}

int cache_data_all(int op)
{
	if (op == K_CACHE_INVD) {
		L1CACHE_CleanInvalidateSystemCache();
	} else {
		return -ENOTSUP;
	}
	return 0;
}

int cache_data_range(void *addr, size_t size, int op)
{
	if (op == K_CACHE_INVD) {
		L1CACHE_CleanInvalidateSystemCacheByRange(*(uint32_t *)addr, size);
	} else {
		return -ENOTSUP;
	}
	return 0;
}

int cache_instr_all(int op)
{
	ARG_UNUSED(op);
	return -ENOTSUP;
}

int cache_instr_range(void *addr, size_t size, int op)
{
	ARG_UNUSED(op);
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	return -ENOTSUP;
}

#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT
size_t cache_data_line_size_get(void)
{
	return -ENOTSUP;
}
#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT */

#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
size_t cache_instr_line_size_get(void)
{
	return -ENOTSUP;
}
#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */
