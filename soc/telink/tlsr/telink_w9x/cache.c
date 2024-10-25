/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief cache manipulation
 *
 * This module contains functions for manipulation cache.
 */

#include <zephyr/cache.h>

#if defined(CONFIG_DCACHE)

#define CSR_CCM_MBEGINADDR		0x7CB
#define CSR_CCM_MCOMMAND		0x7CC
#define CCTL_L1D_VA_INVAL		0
#define CCTL_L1D_VA_WBINVAL		2

#define SRAM_START				0x40000000
#define SRAM_END				0x40020000

int arch_dcache_flush_range(void *start_addr_ptr, size_t size)
{
	if ((uintptr_t)start_addr_ptr >= SRAM_START && (uintptr_t)start_addr_ptr <= SRAM_END) {
		size_t line_size = sys_cache_data_line_size_get();
		uintptr_t end_addr = (uintptr_t)start_addr_ptr + size;
		uintptr_t start_addr = ROUND_DOWN(start_addr_ptr, line_size);

		while (start_addr < end_addr) {
			csr_write(CSR_CCM_MBEGINADDR, start_addr);
			csr_write(CSR_CCM_MCOMMAND, CCTL_L1D_VA_WBINVAL);

			start_addr += line_size;
		}
	}

	return 0;
}

int arch_dcache_invd_range(void *start_addr_ptr, size_t size)
{
	if ((uintptr_t)start_addr_ptr >= SRAM_START && (uintptr_t)start_addr_ptr <= SRAM_END) {
		size_t line_size = sys_cache_data_line_size_get();
		uintptr_t end_addr = (uintptr_t)start_addr_ptr + size;
		uintptr_t start_addr = ROUND_DOWN(start_addr_ptr, line_size);

		while (start_addr < end_addr) {
			csr_write(CSR_CCM_MBEGINADDR, start_addr);
			csr_write(CSR_CCM_MCOMMAND, CCTL_L1D_VA_INVAL);

			start_addr += line_size;
		}
	}

	return 0;
}

void arch_dcache_disable(void)
{
	/* nothing */
}

#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)

void arch_icache_disable(void)
{
	/* nothing */
}

#endif /* CONFIG_ICACHE */

#if defined(CONFIG_ARCH_CACHE)

void arch_cache_init(void)
{
	/* nothing */
}

#endif /* CONFIG_ARCH_CACHE */
