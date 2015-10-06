/* cache.c - cache manipulation */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
DESCRIPTION
This module contains functions for manipulation caches.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/util.h>

#ifdef CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED

#if (CONFIG_CACHE_LINE_SIZE == 0)
#error Cannot use this implementation with a cache line size of 0
#endif

/**
 *
 * @brief Flush a page to main memory
 *
 * No alignment is required for either <virt> or <size>, but since
 * _SysCacheFlush() iterates on the cache lines, a cache line alignment for both
 * is optimal.
 *
 * The cache line size is specified via the CONFIG_CACHE_LINE_SIZE kconfig
 * option.
 *
 * @return N/A
 */

void _SysCacheFlush(VIRT_ADDR virt, size_t size)
{
	int end;

	size = ROUND_UP(size, CONFIG_CACHE_LINE_SIZE);
	end = virt + size;

	for (; virt < end; virt += CONFIG_CACHE_LINE_SIZE) {
		__asm__ volatile("clflush %0;\n\t" :  : "m"(virt));
	}

	__asm__ volatile("mfence;\n\t");
}

#endif /* CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED */
