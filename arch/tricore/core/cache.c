/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Cache manipulation
 *
 * This module contains functions for manipulation caches.
 */

#include "zephyr/kernel.h"
#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <zephyr/sys/util.h>

int arch_dcache_flush_all(void)
{
	return -ENOTSUP;
}

int arch_dcache_invd_all(void)
{
	return -ENOTSUP;
}

int arch_dcache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int arch_dcache_flush_range(void *start_addr, size_t size)
{
	const uintptr_t start = POINTER_TO_UINT(start_addr);
	uintptr_t count = DIV_ROUND_UP(start - ROUND_DOWN(start, CONFIG_DCACHE_LINE_SIZE) + size,
				       CONFIG_DCACHE_LINE_SIZE) -
			  1;
	__asm__ volatile("loop_f:"
			 "cachea.w [%0+]" STRINGIFY(CONFIG_DCACHE_LINE_SIZE) "\n"
									     "loop %1, loop_f\n"
						    : "+a"(start_addr), "+a"(count));
	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	const uintptr_t start = POINTER_TO_UINT(start_addr);
	uintptr_t count = DIV_ROUND_UP(start - ROUND_DOWN(start, CONFIG_DCACHE_LINE_SIZE) + size,
				       CONFIG_DCACHE_LINE_SIZE) -
			  1;
	__asm__ volatile("loop_i:"
			 "cachea.i [%0+]" STRINGIFY(CONFIG_DCACHE_LINE_SIZE) "\n"
									     "loop %1, loop_i\n"
						    : "+a"(start_addr), "+a"(count));
	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	const uintptr_t start = POINTER_TO_UINT(start_addr);
	uintptr_t count = DIV_ROUND_UP(start - ROUND_DOWN(start, CONFIG_DCACHE_LINE_SIZE) + size,
				       CONFIG_DCACHE_LINE_SIZE) -
			  1;
	__asm__ volatile("loop_fi:"
			 "cachea.wi [%0+]" STRINGIFY(CONFIG_DCACHE_LINE_SIZE) "\n"
									      "loop %1, loop_fi\n"
						     : "+a"(start_addr), "+a"(count));
	return 0;
}

int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

int arch_icache_flush_and_invd_all(void)
{
	/* Start instruction cache invalidation via PCON1 */
	cr_write(TRICORE_PCON1, 1);
	/* Wait until invalidation is complete */
	if (!WAIT_FOR((cr_read(TRICORE_PCON1) & 1) == 0, 10, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}
	return 0;
}

int arch_icache_flush_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

int arch_icache_invd_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}
