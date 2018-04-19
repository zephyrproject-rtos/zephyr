/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/alloc.h
 * @brief	zephyr libmetal memory allocattion definitions.
 */

#ifndef __METAL_ALLOC__H__
#error "Include metal/alloc.h instead of metal/zephyr/alloc.h"
#endif

#ifndef __METAL_ZEPHYR_ALLOC__H__
#define __METAL_ZEPHYR_ALLOC__H__

#include <kernel.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
static inline void *metal_allocate_memory(unsigned int size)
{
	return k_malloc(size);
}

static inline void metal_free_memory(void *ptr)
{
	k_free(ptr);
}
#else

void *metal_zephyr_allocate_memory(unsigned int size);
void metal_zephyr_free_memory(void *ptr);

static inline void *metal_allocate_memory(unsigned int size)
{
	return metal_zephyr_allocate_memory(size);
}

static inline void metal_free_memory(void *ptr)
{
	metal_zephyr_free_memory(ptr);
}
#endif /* CONFIG_HEAP_MEM_POOL_SIZE */


#ifdef __cplusplus
}
#endif

#endif /* __METAL_ZEPHYR_ALLOC__H__ */
