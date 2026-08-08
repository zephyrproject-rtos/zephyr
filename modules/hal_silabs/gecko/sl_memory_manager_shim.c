/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shim basic sl_* allocation functions to libc malloc, which again
 * will get redirected to the Zephyr sys_heap.
 */

#include <stdlib.h>

void *sl_malloc(size_t size)
{
	return malloc(size);
}

void *sl_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

void *sl_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void sl_free(void *ptr)
{
	free(ptr);
}
