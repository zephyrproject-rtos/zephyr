/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/osal/osal.h>
#include <zephyr/logging/log.h>

#include "osal_priv.h"

LOG_MODULE_REGISTER(osal, CONFIG_OSAL_LOG_LEVEL);

/*
 * The public heap API always uses the system heap - it mirrors a foreign
 * malloc/free that the consumer asked for explicitly.
 */
void *osal_malloc(size_t size)
{
	return k_malloc(size);
}

void *osal_calloc(size_t n, size_t size)
{
	return k_calloc(n, size);
}

void osal_free(void *ptr)
{
	k_free(ptr);
}

/*
 * Control-block allocation policy. In dynamic mode it uses the system heap; in
 * static mode it uses a dedicated, bounded k_heap so OSAL objects can be
 * created without a system heap (and without unbounded growth). A control
 * block is variable-sized (a queue embeds its ring buffer, etc.), so a byte
 * heap is used rather than a fixed-block slab.
 */
#ifdef CONFIG_OSAL_DYNAMIC_ALLOC

void *osal_cb_alloc(size_t size)
{
	return k_malloc(size);
}

void osal_cb_free(void *ptr)
{
	k_free(ptr);
}

#else

/*
 * Bounded heap for control blocks. Sized as a rough budget of the per-type
 * object count times a typical control-block footprint; tune via
 * CONFIG_OSAL_MAX_OBJECTS.
 */
#define OSAL_CB_HEAP_SIZE (CONFIG_OSAL_MAX_OBJECTS * 7 * 64)

K_HEAP_DEFINE(osal_cb_heap, OSAL_CB_HEAP_SIZE);

void *osal_cb_alloc(size_t size)
{
	void *p = k_heap_alloc(&osal_cb_heap, size, K_NO_WAIT);

	if (p == NULL) {
		LOG_ERR("static cb heap exhausted (CONFIG_OSAL_MAX_OBJECTS=%d)",
			CONFIG_OSAL_MAX_OBJECTS);
	}

	return p;
}

void osal_cb_free(void *ptr)
{
	k_heap_free(&osal_cb_heap, ptr);
}

#endif /* CONFIG_OSAL_DYNAMIC_ALLOC */
