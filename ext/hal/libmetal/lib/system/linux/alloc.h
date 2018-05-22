/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/alloc.h
 * @brief	linux memory allocattion definitions.
 */

#ifndef __METAL_ALLOC__H__
#error "Include metal/alloc.h instead of metal/linux/alloc.h"
#endif

#ifndef __METAL_LINUX_ALLOC__H__
#define __METAL_LINUX_ALLOC__H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *metal_allocate_memory(unsigned int size)
{
	return (malloc(size));
}

static inline void metal_free_memory(void *ptr)
{
	free(ptr);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_LINUX_ALLOC__H__ */
