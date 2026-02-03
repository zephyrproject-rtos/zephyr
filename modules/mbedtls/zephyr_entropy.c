/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/random/random.h>
#include <psa/crypto.h>
#include <mbedtls/platform.h>


#if defined(CONFIG_MBEDTLS_PSA_DRIVER_GET_ENTROPY) || \
    defined(CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
static int get_random_data(uint8_t *output, size_t output_size, bool allow_non_cs)
{
	int ret = -EINVAL;

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
#endif /* CONFIG_MBEDTLS_PSA_DRIVER_GET_ENTROPY || CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG */

#if defined(CONFIG_MBEDTLS_PSA_DRIVER_GET_ENTROPY)
int mbedtls_platform_get_entropy(psa_driver_get_entropy_flags_t flags,
				 size_t *estimate_bits,
				 unsigned char *output, size_t output_size)
{
	ARG_UNUSED(flags);

	if (get_random_data(output, output_size, true) < 0) {
		return -EIO;
	}

	*estimate_bits = 8 * output_size;

	return 0;
}
#endif /* CONFIG_MBEDTLS_PSA_DRIVER_GET_ENTROPY */

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
