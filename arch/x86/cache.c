/* cache.c - cache manipulation */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

/*******************************************************************************
*
* _SysCacheFlush - flush a page to main memory
*
* No alignment is required for either <virt> or <size>, but since
* _SysCacheFlush() iterates on the cache lines, a cache line alignment for both
* is optimal.
*
* The cache line size is specified via the CONFIG_CACHE_LINE_SIZE kconfig
* option.
*
* RETURNS: N/A
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
