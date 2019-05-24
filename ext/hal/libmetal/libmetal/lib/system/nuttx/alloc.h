/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/alloc.h
 * @brief	nuttx libmetal memory allocattion definitions.
 */

#ifndef __METAL_ALLOC__H__
#error "Include metal/alloc.h instead of metal/nuttx/alloc.h"
#endif

#ifndef __METAL_NUTTX_ALLOC__H__
#define __METAL_NUTTX_ALLOC__H__

#include <nuttx/kmalloc.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *metal_allocate_memory(unsigned int size)
{
	return kmm_malloc(size);
}

static inline void metal_free_memory(void *ptr)
{
	kmm_free(ptr);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_NUTTX_ALLOC__H__ */
