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

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/util.h>
#include <toolchain.h>
#include <cache.h>
#include <stdbool.h>

/**
 * @brief Flush cache lines to main memory
 *
 * No alignment is required for either <virt> or <size>, but since
 * sys_cache_flush() iterates on the cache lines, a cache line alignment for
 * both is optimal.
 *
 * The cache line size is specified via the d-cache-line-size DTS property.
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
