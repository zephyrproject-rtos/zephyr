/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>

#ifdef CONFIG_DCACHE
void cache_data_enable(void)
{
	/* Nothing */
}

void cache_data_disable(void)
{
	/* Nothing */
}

int cache_data_flush_all(void)
{
	/* VexRiscv cache is write-through */
	return 0;
}

int cache_data_invd_all(void)
{
	__asm__ volatile(".word(0x500F)\n");

	return 0;
}

int cache_data_flush_and_invd_all(void)
{
	return cache_data_invd_all();
}

int cache_data_flush_range(void *addr __unused, size_t size __unused)
{
	return 0;
}

int cache_data_invd_range(void *addr __unused, size_t size __unused)
{
	return cache_data_invd_all();
}

int cache_data_flush_and_invd_range(void *addr __unused, size_t size __unused)
{
	return cache_data_invd_all();
}
#endif /* CONFIG_DCACHE */

#ifdef CONFIG_ICACHE
void cache_instr_enable(void)
{
	/* Nothing */
}

void cache_instr_disable(void)
{
	/* Nothing */
}

int cache_instr_flush_all(void)
{
	__asm__ volatile("fence.i\n");

	return 0;
}

int cache_instr_invd_all(void)
{
	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	return cache_instr_flush_all();
}

int cache_instr_flush_range(void *addr __unused, size_t size __unused)
{
	return cache_instr_flush_all();
}

int cache_instr_invd_range(void *addr __unused, size_t size __unused)
{
	return 0;
}

int cache_instr_flush_and_invd_range(void *addr __unused, size_t size __unused)
{
	return cache_instr_flush_all();
}
#endif /* CONFIG_ICACHE */
