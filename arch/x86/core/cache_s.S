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
/**
 * @file
 * @brief Cache manipulation
 *
 * This module contains functions for manipulating caches.
 */

#define _ASMLANGUAGE
#include <arch/x86/asm.h>

#ifndef CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED

#if defined(CONFIG_CLFLUSH_DETECT)

	#define CACHE_FLUSH_NAME _cache_flush_wbinvd
	#define CPUID_CFLSH_BIT (1 << 19)

	GTEXT(_is_clflush_available)

SECTION_FUNC(TEXT, _is_clflush_available)
	pushl %ebx
	movl $1, %eax
	cpuid
	movl %edx, %eax
	andl $CPUID_CFLSH_BIT, %eax
	popl %ebx
	ret

#else
	#define CACHE_FLUSH_NAME sys_cache_flush
#endif

	/* externs (internal APIs) */
	GTEXT(CACHE_FLUSH_NAME)

/**
 *
 * @brief Flush a page to main memory
 *
 * This implementation flushes the whole cache.
 *
 * C signature:
 *
 *   void sys_cache_flush (vaddr_t virt, size_t size)
 *
 * Both parameters are ignored in this implementation.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, CACHE_FLUSH_NAME)
	wbinvd
	ret

#endif /* !CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED */

#if defined(CONFIG_CACHE_LINE_SIZE_DETECT)

	#define CPUID_CACHE_LINE_MASK (0xff << 8)

	GTEXT(_cache_line_size_get)

SECTION_FUNC(TEXT, _cache_line_size_get)
	pushl %ebx
	movl $1, %eax
	cpuid
	movl %ebx, %eax
	andl $CPUID_CACHE_LINE_MASK, %eax
	shrl $5,%eax	/* shift right 8 to get value, then multiple by 8
					 * to get cache line size */
	popl %ebx
	ret

#endif /* CONFIG_CACHE_LINE_SIZE_DETECT */
