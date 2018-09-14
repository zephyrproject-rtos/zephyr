/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CACHE_H_
#define ZEPHYR_INCLUDE_CACHE_H_

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _sys_cache_flush_sig(x) void (x)(vaddr_t virt, size_t size)

#if defined(CONFIG_CACHE_FLUSHING)

#if defined(CONFIG_ARCH_CACHE_FLUSH_DETECT)
	typedef _sys_cache_flush_sig(_sys_cache_flush_t);
	extern _sys_cache_flush_t *sys_cache_flush;
#else
	extern _sys_cache_flush_sig(sys_cache_flush);
#endif

#else

/*
 * Provide NOP APIs for systems that do not have caches.
 *
 * An example is the Cortex-M chips. However, the functions are provided so
 * that code that need to manipulate caches can be written in an
 * architecture-agnostic manner. The functions do nothing. The cache line size
 * value is always 0.
 */

static inline _sys_cache_flush_sig(sys_cache_flush)
{
	ARG_UNUSED(virt);
	ARG_UNUSED(size);

	/* do nothing */
}

#endif /* CACHE_FLUSHING */

#if defined(CONFIG_CACHE_LINE_SIZE_DETECT)
	extern size_t sys_cache_line_size;
#elif defined(CONFIG_CACHE_LINE_SIZE)
	#define sys_cache_line_size CONFIG_CACHE_LINE_SIZE
#else
	#define sys_cache_line_size 0
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CACHE_H_ */
