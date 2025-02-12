/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Cache manipulation
 *
 * This module contains functions for manipulation caches.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/cache.h>
#include <stdbool.h>

/* Not Write-through bit */
#define X86_REG_CR0_NW BIT(29)
/* Cache Disable bit */
#define X86_REG_CR0_CD BIT(30)

static inline void z_x86_wbinvd(void)
{
	__asm__ volatile("wbinvd;\n\t" : : : "memory");
}

void arch_dcache_enable(void)
{
	unsigned long cr0 = 0;

	/* Enable write-back caching by clearing the NW and CD bits */
	__asm__ volatile("mov %%cr0, %0;\n\t"
			 "and %1, %0;\n\t"
			 "mov %0, %%cr0;\n\t"
			 : "=r" (cr0)
			 : "i" (~(X86_REG_CR0_NW | X86_REG_CR0_CD)));
}

void arch_dcache_disable(void)
{
	unsigned long cr0 = 0;

	/* Enter the no-fill mode by setting NW=0 and CD=1 */
	__asm__ volatile("mov %%cr0, %0;\n\t"
			 "and %1, %0;\n\t"
			 "or %2, %0;\n\t"
			 "mov %0, %%cr0;\n\t"
			 : "=r" (cr0)
			 : "i" (~(X86_REG_CR0_NW)),
			   "i" (X86_REG_CR0_CD));

	/* Flush all caches */
	z_x86_wbinvd();
}

int arch_dcache_flush_all(void)
{
	z_x86_wbinvd();

	return 0;
}

int arch_dcache_invd_all(void)
{
	z_x86_wbinvd();

	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	z_x86_wbinvd();

	return 0;
}

/**
 * No alignment is required for either <virt> or <size>, but since
 * sys_cache_flush() iterates on the cache lines, a cache line alignment for
 * both is optimal.
 *
 * The cache line size is specified via the d-cache-line-size DTS property.
 */
int arch_dcache_flush_range(void *start_addr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();
	uintptr_t start = (uintptr_t)start_addr;
	uintptr_t end = start + size;

	if (line_size == 0U) {
		return -ENOTSUP;
	}

	end = ROUND_UP(end, line_size);

	for (; start < end; start += line_size) {
		__asm__ volatile("clflush %0;\n\t" :
				"+m"(*(volatile char *)start));
	}

#if defined(CONFIG_X86_MFENCE_INSTRUCTION_SUPPORTED)
	__asm__ volatile("mfence;\n\t":::"memory");
#else
	__asm__ volatile("lock; addl $0,-4(%%esp);\n\t":::"memory", "cc");
#endif
	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	return arch_dcache_flush_range(start_addr, size);
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	return arch_dcache_flush_range(start_addr, size);
}
