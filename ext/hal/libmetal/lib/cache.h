/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	cache.h
 * @brief	CACHE operation primitives for libmetal.
 */

#ifndef __METAL_CACHE__H__
#define __METAL_CACHE__H__

#include <metal/system/@PROJECT_SYSTEM@/cache.h>

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup cache CACHE Interfaces
 *  @{ */

/**
 * @brief flush specified data cache
 *
 * @param[in] addr start memory logical address
 * @param[in] len  length of memory
 *                 If addr is NULL, and len is 0,
 *                 It will flush the whole data cache.
 */
static inline void metal_cache_flush(void *addr, unsigned int len)
{
	return __metal_cache_flush(addr, len);
}

/**
 * @brief invalidate specified data cache
 *
 * @param[in] addr start memory logical address
 * @param[in] len  length of memory
 *                 If addr is NULL, and len is 0,
 *                 It will invalidate the whole data cache.
 */
static inline void metal_cache_invalidate(void *addr, unsigned int len)
{
	return __metal_cache_invalidate(addr, len);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_CACHE__H__ */
