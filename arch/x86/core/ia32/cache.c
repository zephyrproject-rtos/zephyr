/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Cache manipulation
 *
 * This module contains functions for manipulation caches.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/util.h>
#include <toolchain.h>
#include <cache.h>
#include <stdbool.h>
#include <cache.h>

/*
 * these functions are defined in cache_s.S
 */

extern int z_is_clflush_available(void);
extern void z_cache_flush_wbinvd(vaddr_t addr, size_t len);
extern size_t z_cache_line_size_get(void);

#if defined(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED) || \
	defined(CONFIG_CLFLUSH_DETECT)

#if (CONFIG_CACHE_LINE_SIZE == 0) && !defined(CONFIG_CACHE_LINE_SIZE_DETECT)
#error Cannot use this implementation with a cache line size of 0
#endif

/**
 *
 * @brief Flush cache lines to main memory
 *
 * No alignment is required for either <virt> or <size>, but since
 * sys_cache_flush() iterates on the cache lines, a cache line alignment for
 * both is optimal.
 *
 * The cache line size is specified either via the CONFIG_CACHE_LINE_SIZE
 * kconfig option or it is detected at runtime.
 *
 * @return N/A
 */

void arch_dcache_flush(void *start_addr, size_t size)
{
	uintptr_t end;

	size = ROUND_UP(size, sys_cache_line_size);
	end = (uintptr_t)start_addr + size;

	for (; (uintptr_t)start_addr < end; (uintptr_t)start_addr += sys_cache_line_size) {
		__asm__ volatile("clflush %0;\n\t" :  : "m"((uintptr_t)start_addr));
	}

	__asm__ volatile("mfence;\n\t");
}

#endif /* CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED || CLFLUSH_DETECT */

#if defined(CONFIG_CLFLUSH_DETECT) || defined(CONFIG_CACHE_LINE_SIZE_DETECT)

#include <init.h>

#if defined(CONFIG_CACHE_LINE_SIZE_DETECT)
size_t sys_cache_line_size;
static void init_cache_line_size(void)
{
	sys_cache_line_size = z_cache_line_size_get();
}
#else
#define init_cache_line_size() do { } while ((0))
#endif

size_t arch_cache_line_size_get(void)
{
#if defined(CONFIG_CACHE_LINE_SIZE_DETECT)
	return sys_cache_line_size;
#else
	return 0;
#endif
}

static int init_cache(const struct device *unused)
{
	ARG_UNUSED(unused);

	init_cache_line_size();

	return 0;
}

SYS_INIT(init_cache, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_CLFLUSH_DETECT || CONFIG_CACHE_LINE_SIZE_DETECT */
