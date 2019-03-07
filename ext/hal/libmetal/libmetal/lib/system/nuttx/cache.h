/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/cache.h
 * @brief	NuttX cache operation primitives for libmetal.
 */

#ifndef __METAL_CACHE__H__
#error "Include metal/cache.h instead of metal/nuttx/cache.h"
#endif

#ifndef __METAL_NUTTX_CACHE__H__
#define __METAL_NUTTX_CACHE__H__

#include <nuttx/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void __metal_cache_flush(void *addr, unsigned int len)
{
	up_clean_dcache((uintptr_t)addr, (uintptr_t)addr + len);
}

static inline void __metal_cache_invalidate(void *addr, unsigned int len)
{
	up_invalidate_dcache((uintptr_t)addr, (uintptr_t)addr + len);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_NUTTX_CACHE__H__ */
