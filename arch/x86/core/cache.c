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
