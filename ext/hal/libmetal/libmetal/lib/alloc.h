/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	alloc.h
 * @brief	Memory allocation handling primitives for libmetal.
 */

#ifndef __METAL_ALLOC__H__
#define __METAL_ALLOC__H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup Memory Allocation Interfaces
 *  @{ */

/**
 * @brief      allocate requested memory size
 *             return a pointer to the allocated memory
 *
 * @param[in]  size        size in byte of requested memory
 * @return     memory pointer, or 0 if it failed to allocate
 */
static inline void *metal_allocate_memory(unsigned int size);

/**
 * @brief      free the memory previously allocated
 *
 * @param[in]  ptr       pointer to memory 
 */
static inline void metal_free_memory(void *ptr);

#include <metal/system/@PROJECT_SYSTEM@/alloc.h>

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_ALLOC__H__ */
