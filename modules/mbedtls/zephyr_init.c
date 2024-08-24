/** @file
 * @brief mbed TLS initialization
 *
 * Initialize the mbed TLS library like setup the heap etc.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <mbedtls/platform_time.h>

#include <mbedtls/debug.h>

#if defined(CONFIG_MBEDTLS)
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */
#endif

#if defined(CONFIG_MBEDTLS_ENABLE_HEAP) && \
	defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include <mbedtls/memory_buffer_alloc.h>

#if !defined(CONFIG_MBEDTLS_HEAP_SIZE)
#error "Please set heap size to be used. Set value to CONFIG_MBEDTLS_HEAP_SIZE \
option."
#endif

static unsigned char _mbedtls_heap[CONFIG_MBEDTLS_HEAP_SIZE];

static void init_heap(void)
{
	mbedtls_memory_buffer_alloc_init(_mbedtls_heap, sizeof(_mbedtls_heap));
}
#else
#define init_heap(...)
#endif /* CONFIG_MBEDTLS_ENABLE_HEAP && MBEDTLS_MEMORY_BUFFER_ALLOC_C */

static int _mbedtls_init(void)
{

	init_heap();

#if defined(CONFIG_MBEDTLS_DEBUG_LEVEL)
	mbedtls_debug_set_threshold(CONFIG_MBEDTLS_DEBUG_LEVEL);
#endif

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT)
	if (psa_crypto_init() != PSA_SUCCESS) {
		return -EIO;
	}
#endif

	return 0;
}

#if defined(CONFIG_MBEDTLS_INIT)
SYS_INIT(_mbedtls_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

/* if CONFIG_MBEDTLS_INIT is not defined then this function
 * should be called by the platform before any mbedtls functionality
 * is used
 */
int mbedtls_init(void)
{
	return _mbedtls_init();
}

/* TLS 1.3 ticket lifetime needs a timing interface */
mbedtls_ms_time_t mbedtls_ms_time(void)
{
	return (mbedtls_ms_time_t)k_uptime_get();
}
