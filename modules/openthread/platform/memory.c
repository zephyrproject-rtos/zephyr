/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <openthread/platform/memory.h>

#ifdef CONFIG_OPENTHREAD_EXTERNAL_HEAP_SHARED_MULTI_HEAP

#include <zephyr/multi_heap/shared_multi_heap.h>

#include <string.h>

void *otPlatCAlloc(size_t aNum, size_t aSize)
{
	size_t total = aNum * aSize;
	void *ptr = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, total);

	if (ptr != NULL) {
		memset(ptr, 0, total);
	}
	return ptr;
}

void otPlatFree(void *aPtr)
{
	shared_multi_heap_free(aPtr);
}
#else

#include <openthread/platform/memory.h>

#include <stdlib.h>

void *otPlatCAlloc(size_t aNum, size_t aSize)
{
	return calloc(aNum, aSize);
}

void otPlatFree(void *aPtr)
{
	free(aPtr);
}

#endif /* CONFIG_OPENTHREAD_EXTERNAL_HEAP_SHARED_MULTI_HEAP */
