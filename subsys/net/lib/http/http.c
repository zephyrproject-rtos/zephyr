/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_HTTP)
#define SYS_LOG_DOMAIN "http"
#define NET_LOG_ENABLED 1
#endif

#include <stdbool.h>

#include <net/http.h>

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include <mbedtls/memory_buffer_alloc.h>
static unsigned char heap[CONFIG_HTTPS_HEAP_SIZE];

void http_heap_init(void)
{
	static bool heap_init;

	if (!heap_init) {
		heap_init = true;
		mbedtls_memory_buffer_alloc_init(heap, sizeof(heap));
	}
}
#endif /* MBEDTLS_MEMORY_BUFFER_ALLOC_C */
