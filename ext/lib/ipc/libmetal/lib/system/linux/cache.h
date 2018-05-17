/*
 * Copyright (c) 2018, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/cache.h
 * @brief	Linux cache operation primitives for libmetal.
 */

#ifndef __METAL_CACHE__H__
#error "Include metal/cache.h instead of metal/linux/cache.h"
#endif

#ifndef __METAL_LINUX_CACHE__H__
#define __METAL_LINUX_CACHE__H__

#include <metal/utilities.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void __metal_cache_flush(void *addr, unsigned int len)
{
	/** Do nothing.
	 *  Do not flush cache from Linux userspace.
	 */
	metal_unused(addr);
	metal_unused(len);
	return;
}

static inline void __metal_cache_invalidate(void *addr, unsigned int len)
{
	/** Do nothing.
	 *  Do not flush cache from Linux userspace.
	 */
	metal_unused(addr);
	metal_unused(len);
	return;
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_LINUX_CACHE__H__ */
