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

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
size_t sys_cache_line_size;
#endif

#if defined(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED) || \
	defined(CONFIG_CLFLUSH_DETECT)

/**
 *
 * @brief Flush cache lines to main memory
 *
 * No alignment is required for either <virt> or <size>, but since
 * sys_cache_flush() iterates on the cache lines, a cache line alignment for
 * both is optimal.
 *
 * The cache line size is specified either via the CONFIG_DCACHE_LINE_SIZE
 * kconfig option or it is detected at runtime.
 *
 * @return N/A
 */

static void arch_dcache_flush(void *start_addr, size_t size)
{
	size_t line_size = sys_dcache_line_size_get();
	uintptr_t start = (uintptr_t)start_addr;
	uintptr_t end;

	if (line_size == 0U) {
		return;
	}

	size = ROUND_UP(size, line_size);
	end = start + size;

	for (; start < end; start += line_size) {
		__asm__ volatile("clflush %0;\n\t" :  : "m"(start));
	}

	__asm__ volatile("mfence;\n\t");
}

int arch_dcache_range(void *addr, size_t size, int op)
{
	if (op & K_CACHE_WB) {
		arch_dcache_flush(addr, size);
		return 0;
	}

	return -ENOTSUP;
}

#endif /* CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED || CLFLUSH_DETECT */

#include <init.h>

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
static void init_cache_line_size(void)
{
	sys_cache_line_size = z_cache_line_size_get();
}

size_t arch_cache_line_size_get(void)
{
	return sys_cache_line_size;
}
#endif

static int init_dcache(const struct device *unused)
{
	ARG_UNUSED(unused);

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
	init_cache_line_size();
#endif
	return 0;
}

SYS_INIT(init_dcache, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
