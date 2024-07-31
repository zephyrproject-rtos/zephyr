/** @file
 * @brief mbed TLS initialization
 *
 * Initialize the mbed TLS library like setup the heap etc.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/random/random.h>
#include <mbedtls/entropy.h>
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

#if defined(CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR) || defined(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
static int get_random_data(uint8_t *output, size_t output_size, bool allow_non_cs)
{
	int ret = MBEDTLS_ERR_ENTROPY_NO_SOURCES_DEFINED;

#if defined(CONFIG_CSPRNG_ENABLED)
	ret = sys_csrand_get(output, output_size);
	if (ret == 0) {
		return 0;
	}
#endif /* CONFIG_CSPRNG_ENABLED */

	if (allow_non_cs) {
		sys_rand_get(output, output_size);
		ret = 0;
	}

	return ret;
}
#endif /* CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR || CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG */

#if defined(CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR)
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len,
			  size_t *olen)
{
	int ret;
	uint16_t request_len = len > UINT16_MAX ? UINT16_MAX : len;

	ARG_UNUSED(data);

	if (output == NULL || olen == NULL || len == 0) {
		return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
	}

	ret = get_random_data(output, len, true);
	if (ret < 0) {
		return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
	}

	*olen = request_len;

	return 0;
}
#endif /* CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR */

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
psa_status_t mbedtls_psa_external_get_random(
	mbedtls_psa_external_random_context_t *context,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	(void) context;
	int ret;

	ret = get_random_data(output, output_size,
		IS_ENABLED(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG_ALLOW_NON_CSPRNG));
	if (ret != 0) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	*output_length = output_size;

	return PSA_SUCCESS;
}
#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG */
