/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <cmsis_os.h>
#include <string.h>

#define TIME_OUT	K_MSEC(100)

/**
 * @brief Create and Initialize a memory pool.
 */
osPoolId osPoolCreate(const osPoolDef_t *pool_def)
{
	if (k_is_in_isr()) {
		return NULL;
	}

	return (osPoolId)pool_def;
}

/**
 * @brief Allocate a memory block from a memory pool.
 */
void *osPoolAlloc(osPoolId pool_id)
{
	osPoolDef_t *osPool = (osPoolDef_t *)pool_id;
	void *ptr;

	if (k_mem_slab_alloc((struct k_mem_slab *)(osPool->pool),
				&ptr, TIME_OUT) == 0) {
		return ptr;
	} else {
		return NULL;
	}
}

/**
 * @brief Allocate a memory block from a memory pool and set it to zero.
 */
void *osPoolCAlloc(osPoolId pool_id)
{
	osPoolDef_t *osPool = (osPoolDef_t *)pool_id;
	void *ptr;

	if (k_mem_slab_alloc((struct k_mem_slab *)(osPool->pool),
				&ptr, TIME_OUT) == 0) {
		(void)memset(ptr, 0, osPool->item_sz);
		return ptr;
	} else {
		return NULL;
	}
}

/**
 * @brief Return an allocated memory block back to a specific memory pool.
 */
osStatus osPoolFree(osPoolId pool_id, void *block)
{
	osPoolDef_t *osPool = (osPoolDef_t *)pool_id;

	/* Note: Below 2 error codes are not supported.
	 *       osErrorValue: block does not belong to the memory pool.
	 *       osErrorParameter: a parameter is invalid or outside of a
	 *                         permitted range.
	 */

	k_mem_slab_free((struct k_mem_slab *)(osPool->pool), (void *)&block);

	return osOK;
}
