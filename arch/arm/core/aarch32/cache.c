/*
 * SPDX-License-Identifier: Apache-2.0
 */

/* cache.c - cache support for AARCH32 CPUs */

/**
 * @file
 * @brief cache manipulation
 *
 * This module contains functions for manipulation of the cache.
 * Based on ARM documentation from:
 *   - https://developer.arm.com/documentation/ddi0301/h/Babhejba
 */

#include <zephyr/cache.h>

/*
 * operation for data cache by virtual address to PoC
 * ops:  K_CACHE_INVD: invalidate
 *	 K_CACHE_WB: clean
 *	 K_CACHE_WB_INVD: clean and invalidate
 */
int arch_dcache_range(void *addr, size_t size, int op)
{
	uintptr_t start_addr = (uintptr_t)addr;
	uintptr_t end_addr = (uintptr_t)addr + size;

	start_addr /= CONFIG_DCACHE_LINE_SIZE;
	start_addr *= CONFIG_DCACHE_LINE_SIZE;

	for (; start_addr < end_addr; start_addr += CONFIG_DCACHE_LINE_SIZE) {
		switch (op) {
		case K_CACHE_WB:
			__asm__ volatile("mcr p15, #0, %0, c7, c10, #1" : : "r"(start_addr));
			break;
		case K_CACHE_WB_INVD:
			__asm__ volatile("mcr p15, #0, %0, c7, c14, #1" : : "r"(start_addr));
			break;
		case K_CACHE_INVD:
			__asm__ volatile("mcr p15, #0, %0, c7, c6, #1" : : "r"(start_addr));
			break;
		default:
			return -ENOTSUP;
		}
	}
	__DSB();
	return 0;
}

/*
 * operation for all data cache
 * ops:  K_CACHE_INVD: invalidate
 *	 K_CACHE_WB: clean
 *	 K_CACHE_WB_INVD: clean and invalidate
 */
int arch_dcache_all(int op)
{
	switch (op) {
	case K_CACHE_WB:
		__asm__ volatile("mcr p15, #0, %0, c7, c10, #0" : : "r"(0));
		break;
	case K_CACHE_WB_INVD:
		__asm__ volatile("mcr p15, #0, %0, c7, c14, #0" : : "r"(0));
		break;
	case K_CACHE_INVD:
		__asm__ volatile("mcr p15, #0, %0, c7, c6, #0" : : "r"(0));
		break;
	default:
		return -ENOTSUP;
	}
	__DSB();
	return 0;
}

/*
 * operation for instruction cache by virtual address to PoC
 * ops:  K_CACHE_INVD: invalidate
 */
int arch_icache_range(void *addr, size_t size, int op)
{
	uintptr_t start_addr = (uintptr_t)addr;
	uintptr_t end_addr = (uintptr_t)addr + size;

	start_addr /= CONFIG_DCACHE_LINE_SIZE;
	start_addr *= CONFIG_DCACHE_LINE_SIZE;

	if (op != K_CACHE_WB_INVD)
		return -ENOTSUP;

	for (; start_addr < end_addr; start_addr += CONFIG_DCACHE_LINE_SIZE)
		__asm__ volatile("mcr p15, #0, %0, c7, c5, #1" : : "r"(start_addr));
	__ISB();
	return 0;
}

/*
 * operation for all instruction cache
 * ops:  K_CACHE_INVD: invalidate
 */
int arch_icache_all(int op)
{
	if (op != K_CACHE_WB_INVD)
		return -ENOTSUP;

	__asm__ volatile("mcr p15, #0, %0, c7, c5, #0" : : "r"(0));
	__ISB();
	return 0;
}
