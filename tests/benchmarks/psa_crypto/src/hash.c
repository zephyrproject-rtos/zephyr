/*
 * Copyright (C) 2025, BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <psa/crypto.h>

#define BUF_SIZE 1024

static uint8_t in_buf[BUF_SIZE];
static uint8_t out_buf[BUF_SIZE];

static void suite_setup(void)
{
	memset(in_buf, 0xaa, sizeof(in_buf));
}

ZTEST_BENCHMARK_SUITE(psa_crypto_hash, suite_setup, NULL);

ZTEST_BENCHMARK_TIMED(psa_crypto_hash, sha1, 1000U, NULL, NULL)
{
	size_t out_len;

	psa_hash_compute(PSA_ALG_SHA_1, in_buf, sizeof(in_buf),
			 out_buf, sizeof(out_buf), &out_len);
}

ZTEST_BENCHMARK_TIMED(psa_crypto_hash, sha224, 1000U, NULL, NULL)
{
	size_t out_len;

	psa_hash_compute(PSA_ALG_SHA_224, in_buf, sizeof(in_buf),
			 out_buf, sizeof(out_buf), &out_len);
}

ZTEST_BENCHMARK_TIMED(psa_crypto_hash, sha256, 1000U, NULL, NULL)
{
	size_t out_len;

	psa_hash_compute(PSA_ALG_SHA_256, in_buf, sizeof(in_buf),
			 out_buf, sizeof(out_buf), &out_len);
}

ZTEST_BENCHMARK_TIMED(psa_crypto_hash, sha384, 1000U, NULL, NULL)
{
	size_t out_len;

	psa_hash_compute(PSA_ALG_SHA_384, in_buf, sizeof(in_buf),
			 out_buf, sizeof(out_buf), &out_len);
}

ZTEST_BENCHMARK_TIMED(psa_crypto_hash, sha512, 1000U, NULL, NULL)
{
	size_t out_len;

	psa_hash_compute(PSA_ALG_SHA_512, in_buf, sizeof(in_buf),
			 out_buf, sizeof(out_buf), &out_len);
}
