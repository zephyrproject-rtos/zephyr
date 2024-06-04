/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Simple test to show support for secp256r1 curve with either MbedTLS and
 * TinyCrypt. Operations are pretty simple:
 * - generate 2 keys
 * - perform key agreement.
 * The idea is to provide a way to compare memory footprint for the very
 * same kind of implemented feature between the 2 crypto libraries.
 */

#include <zephyr/ztest.h>

#if defined(CONFIG_MBEDTLS)
#if defined(CONFIG_MBEDTLS_PSA_P256M_DRIVER_RAW)
#include "p256-m.h"
#else /* CONFIG_MBEDTLS_PSA_P256M_DRIVER_RAW */
#include "psa/crypto.h"
#endif /* CONFIG_MBEDTLS_PSA_P256M_DRIVER_RAW */
#else /* CONFIG_MBEDTLS */
#include "zephyr/random/random.h"
#include "tinycrypt/constants.h"
#include "tinycrypt/ecc.h"
#include "tinycrypt/ecc_dh.h"
#endif /* CONFIG_MBEDTLS */

#if defined(CONFIG_MBEDTLS)
#if defined(CONFIG_MBEDTLS_PSA_P256M_DRIVER_RAW)
ZTEST_USER(test_fn, test_mbedtls)
{
	int ret;
	uint8_t public_key_1[64], public_key_2[64];
	uint8_t private_key_1[32], private_key_2[32];
	uint8_t secret[32];

	ret = p256_gen_keypair(private_key_1, public_key_1);
	zassert_equal(ret, P256_SUCCESS, "Unable to generate 1st EC key (%d)", ret);

	ret = p256_gen_keypair(private_key_2, public_key_2);
	zassert_equal(ret, P256_SUCCESS, "Unable to generate 2nd EC key (%d)", ret);

	ret = p256_ecdh_shared_secret(secret, private_key_1, public_key_2);
	zassert_equal(ret, P256_SUCCESS, "Unable to compute the shared secret (%d)", ret);
}
#else /* CONFIG_MBEDTLS_PSA_P256M_DRIVER_RAW */
ZTEST_USER(test_fn, test_mbedtls)
{
	psa_status_t status;
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	mbedtls_svc_key_id_t key_id_1 = MBEDTLS_SVC_KEY_ID_INIT;
	mbedtls_svc_key_id_t key_id_2 = MBEDTLS_SVC_KEY_ID_INIT;
	uint8_t public_key_2[65];
	size_t public_key_2_len;
	uint8_t secret[32];
	size_t secret_len;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attr, 256);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&key_attr, PSA_ALG_ECDH);

	status = psa_generate_key(&key_attr, &key_id_1);
	zassert_equal(status, PSA_SUCCESS, "Unable to generate 1st EC key (%d)", status);

	status = psa_generate_key(&key_attr, &key_id_2);
	zassert_equal(status, PSA_SUCCESS, "Unable to generate 1st EC key (%d)", status);

	status = psa_export_public_key(key_id_2, public_key_2, sizeof(public_key_2),
					&public_key_2_len);
	zassert_equal(status, PSA_SUCCESS, "Unable to export public key (%d)", status);

	status = psa_raw_key_agreement(PSA_ALG_ECDH, key_id_1, public_key_2, public_key_2_len,
					secret, sizeof(secret), &secret_len);
	zassert_equal(status, PSA_SUCCESS, "Unable to compute shared secret (%d)", status);
}
#endif /* CONFIG_MBEDTLS_PSA_P256M_DRIVER_RAW */
#else /* CONFIG_MBEDTLS */
ZTEST_USER(test_fn, test_tinycrypt)
{
	uint8_t public_key_1[64], public_key_2[64];
	uint8_t private_key_1[32], private_key_2[32];
	uint8_t secret[32];
	int ret;

	ret = uECC_make_key(public_key_1, private_key_1, &curve_secp256r1);
	zassert_equal(ret, TC_CRYPTO_SUCCESS, "Unable to generate 1st EC key (%d)", ret);

	ret = uECC_make_key(public_key_2, private_key_2, &curve_secp256r1);
	zassert_equal(ret, TC_CRYPTO_SUCCESS, "Unable to generate 2nd EC key (%d)", ret);

	ret = uECC_valid_public_key(public_key_2, &curve_secp256r1);
	zassert_equal(ret, 0, "Invalid public key (%d)", ret);

	ret = uECC_shared_secret(public_key_2, private_key_1, secret, &curve_secp256r1);
	zassert_equal(ret, TC_CRYPTO_SUCCESS, "Unable to compute the shared secret (%d)", ret);
}

int default_CSPRNG(uint8_t *dst, unsigned int len)
{
	return (sys_csrand_get(dst, len) == 0);
}
#endif /* CONFIG_MBEDTLS */

ZTEST_SUITE(test_fn, NULL, NULL, NULL, NULL, NULL);
