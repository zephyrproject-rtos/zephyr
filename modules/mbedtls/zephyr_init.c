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
#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <mbedtls/platform_time.h>
#include <errno.h>

#ifdef CONFIG_MBEDTLS_THREADING_ALT
#include <threading_alt.h>
#include <mbedtls/threading.h>
#endif

#include <mbedtls/debug.h>

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT)
#include <psa/crypto.h>
#endif

#if defined(CONFIG_MBEDTLS_ENABLE_HEAP)
#include <mbedtls/memory_buffer_alloc.h>

#ifdef CONFIG_MBEDTLS_HEAP_CUSTOM_SECTION
#define HEAP_MEM_ATTRIBUTES Z_GENERIC_SECTION(.mbedtls_heap)
#else
#define HEAP_MEM_ATTRIBUTES
#endif /* CONFIG_MBEDTLS_HEAP_CUSTOM_SECTION */
static unsigned char _mbedtls_heap[CONFIG_MBEDTLS_HEAP_SIZE] HEAP_MEM_ATTRIBUTES;

static void init_heap(void)
{
	mbedtls_memory_buffer_alloc_init(_mbedtls_heap, sizeof(_mbedtls_heap));
}
#else
#define init_heap(...)
#endif /* CONFIG_MBEDTLS_ENABLE_HEAP */

#if defined(CONFIG_MBEDTLS_THREADING_ALT)
static int mbedtls_alt_mutex_init(mbedtls_platform_mutex_t *mutex)
{
	if (k_mutex_init(mutex) < 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static void mbedtls_alt_mutex_destroy(mbedtls_platform_mutex_t *mutex __unused)
{
}

static int mbedtls_alt_mutex_lock(mbedtls_platform_mutex_t *mutex)
{
	if (k_mutex_lock(mutex, K_FOREVER) < 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static int mbedtls_alt_mutex_unlock(mbedtls_platform_mutex_t *mutex)
{
	if (k_mutex_unlock(mutex) < 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static int mbedtls_alt_cond_init(mbedtls_platform_condition_variable_t *condvar)
{
	if (k_condvar_init(condvar) < 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static void mbedtls_alt_cond_destroy(mbedtls_platform_condition_variable_t *condvar __unused)
{
}

static int mbedtls_alt_cond_signal(mbedtls_platform_condition_variable_t *condvar)
{
	if (k_condvar_signal(condvar) < 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static int mbedtls_alt_cond_broadcast(mbedtls_platform_condition_variable_t *condvar)
{
	if (k_condvar_broadcast(condvar) < 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static int mbedtls_alt_cond_wait(mbedtls_platform_condition_variable_t *condvar,
				 mbedtls_platform_mutex_t *mutex)
{
	if (k_condvar_wait(condvar, mutex, K_FOREVER) < 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static void init_threading(void)
{
	mbedtls_threading_set_alt(mbedtls_alt_mutex_init, mbedtls_alt_mutex_destroy,
				  mbedtls_alt_mutex_lock, mbedtls_alt_mutex_unlock,
				  mbedtls_alt_cond_init, mbedtls_alt_cond_destroy,
				  mbedtls_alt_cond_signal, mbedtls_alt_cond_broadcast,
				  mbedtls_alt_cond_wait);
}
#else
static void init_threading(void)
{
}
#endif /* CONFIG_MBEDTLS_THREADING_ALT */

static int _mbedtls_init(void)
{

	init_heap();

	init_threading();

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
