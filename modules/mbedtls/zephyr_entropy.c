/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/random/random.h>
#include <mbedtls/entropy.h>
#include <psa/crypto.h>


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
