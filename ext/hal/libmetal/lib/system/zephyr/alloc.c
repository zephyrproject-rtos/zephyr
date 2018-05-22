/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/alloc.c
 * @brief	Zephyr libmetal memory allocation handling.
 */

#include <metal/alloc.h>
#include <metal/compiler.h>

#if (CONFIG_HEAP_MEM_POOL_SIZE <= 0)

void* metal_weak metal_zephyr_allocate_memory(unsigned int size)
{
	(void)size;
	return NULL;
}

void metal_weak metal_zephyr_free_memory(void *ptr)
{
	(void)ptr;
}

#endif /* CONFIG_HEAP_MEM_POOL_SIZE */
