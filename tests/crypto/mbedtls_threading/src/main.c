/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <mbedtls/memory_buffer_alloc.h>
#include <mbedtls/platform.h>
#include <psa/crypto.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_MBEDTLS_THREADING_C));
BUILD_ASSERT(IS_ENABLED(CONFIG_MBEDTLS_THREADING_ALT));
BUILD_ASSERT(IS_ENABLED(CONFIG_MBEDTLS_THREADING_ZEPHYR));
BUILD_ASSERT(!IS_ENABLED(CONFIG_MBEDTLS_THREADING_CUSTOM));

#define NUM_WORKERS       3
#define WORKER_STACK_SIZE 4096
#define CRYPTO_ITERATIONS 32

static K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, NUM_WORKERS, WORKER_STACK_SIZE);
static struct k_thread worker_threads[NUM_WORKERS];
static K_SEM_DEFINE(crypto_start_sem, 0, NUM_WORKERS);

struct crypto_stress_ctx {
	psa_key_id_t shared_key_id;
	uint32_t completed_iters[NUM_WORKERS];
};

static void crypto_stress_worker(void *p1, void *p2, void *p3)
{
	static const uint8_t stress_hash_msg[32] = {
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
		0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
		0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	};
	/* SHA-256("abcdefghijklmnopqrstuvwxyz012345") */
	static const uint8_t expected_stress_hash[32] = {
		0x65, 0x3b, 0xb1, 0x24, 0x5e, 0x82, 0x8f, 0xcd, 0xa4, 0xfa, 0x53,
		0xfc, 0xd5, 0xa3, 0xde, 0xf5, 0xbd, 0x76, 0x54, 0xe6, 0x51, 0xf5,
		0x4b, 0x41, 0x32, 0xb7, 0x3d, 0x74, 0xe6, 0x44, 0x35, 0xc4,
	};
	struct crypto_stress_ctx *ctx = p1;
	uint32_t tid = POINTER_TO_UINT(p2);

	ARG_UNUSED(p3);

	zassert_ok(k_sem_take(&crypto_start_sem, K_FOREVER));

	for (uint32_t iter = 0U; iter < CRYPTO_ITERATIONS; iter++) {
		size_t alloc_len = 24U + (((iter * 13U) + (tid * 17U)) % 72U);
		uint8_t pattern = (uint8_t)(0x5aU ^ (tid << 4) ^ iter);
		uint8_t *heap_buf;
		uint8_t plain[32];
		uint8_t aad[8];
		uint8_t nonce[12];
		uint8_t cipher[32 + 16];
		uint8_t decrypted[32];
		uint8_t hash[32];
		uint8_t local_key_bytes[16];
		size_t out_len = 0U;
		size_t dec_len = 0U;
		size_t hash_len = 0U;
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_key_id_t local_key_id = PSA_KEY_ID_NULL;

		/* 1. Exercise _mbedtls_heap mutex under concurrent contention. */
		heap_buf = mbedtls_calloc(1U, alloc_len);
		zassert_not_null(heap_buf, "mbedtls_calloc failed");
		for (size_t i = 0U; i < alloc_len; i++) {
			zassert_equal(heap_buf[i], 0U);
			heap_buf[i] = (uint8_t)(pattern + (uint8_t)i);
		}

		/* 2. Exercise concurrent PSA RNG calls via psa_generate_random(). */
		zassert_equal(psa_generate_random(nonce, sizeof(nonce)), PSA_SUCCESS);

		for (size_t i = 0U; i < sizeof(plain); i++) {
			plain[i] = (uint8_t)((tid * 31U) + iter + i);
		}
		for (size_t i = 0U; i < sizeof(aad); i++) {
			aad[i] = (uint8_t)(0xa0U + tid + i);
		}

		/* 3. Concurrent AEAD encrypt/decrypt on shared key slot. */
		zassert_equal(psa_aead_encrypt(ctx->shared_key_id, PSA_ALG_GCM, nonce,
					       sizeof(nonce), aad, sizeof(aad), plain,
					       sizeof(plain), cipher, sizeof(cipher), &out_len),
			      PSA_SUCCESS);
		zassert_equal(out_len, sizeof(cipher));

		if ((iter & 1U) == 0U) {
			k_yield();
		}

		dec_len = 0U;
		zassert_equal(psa_aead_decrypt(ctx->shared_key_id, PSA_ALG_GCM, nonce,
					       sizeof(nonce), aad, sizeof(aad), cipher,
					       sizeof(cipher), decrypted, sizeof(decrypted),
					       &dec_len),
			      PSA_SUCCESS);
		zassert_equal(dec_len, sizeof(plain));
		zassert_mem_equal(decrypted, plain, sizeof(plain));

		/* 4. Import, round-trip encrypt/decrypt, and destroy volatile key slot. */
		for (size_t i = 0U; i < sizeof(local_key_bytes); i++) {
			local_key_bytes[i] = (uint8_t)(0x33U ^ (tid * 19U) ^ iter ^ i);
		}

		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
		psa_set_key_algorithm(&attr, PSA_ALG_GCM);
		psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
		psa_set_key_bits(&attr, 128U);

		zassert_equal(psa_import_key(&attr, local_key_bytes, sizeof(local_key_bytes),
					     &local_key_id),
			      PSA_SUCCESS);
		zassert_equal(psa_hash_compute(PSA_ALG_SHA_256, stress_hash_msg,
					       sizeof(stress_hash_msg), hash, sizeof(hash),
					       &hash_len),
			      PSA_SUCCESS);
		zassert_equal(hash_len, sizeof(hash));
		zassert_mem_equal(hash, expected_stress_hash, sizeof(hash));

		out_len = 0U;
		zassert_equal(psa_aead_encrypt(local_key_id, PSA_ALG_GCM, nonce, sizeof(nonce),
					       hash, 12U, plain, 16U, cipher, sizeof(cipher),
					       &out_len),
			      PSA_SUCCESS);
		zassert_equal(out_len, 32U);

		memset(decrypted, 0, sizeof(decrypted));
		dec_len = 0U;
		zassert_equal(psa_aead_decrypt(local_key_id, PSA_ALG_GCM, nonce, sizeof(nonce),
					       hash, 12U, cipher, out_len, decrypted,
					       sizeof(decrypted), &dec_len),
			      PSA_SUCCESS);
		zassert_equal(dec_len, 16U);
		zassert_mem_equal(decrypted, plain, 16U);

		zassert_equal(psa_destroy_key(local_key_id), PSA_SUCCESS);

		/* Verify heap block integrity before freeing. */
		for (size_t i = 0U; i < alloc_len; i++) {
			zassert_equal(heap_buf[i], (uint8_t)(pattern + (uint8_t)i));
		}
		mbedtls_free(heap_buf);

		ctx->completed_iters[tid]++;
	}
}

ZTEST(mbedtls_threading, test_psa_concurrent_crypto_and_heap_stress)
{
	static const uint8_t shared_key_bytes[16] = {
		0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
		0x98, 0xa9, 0xba, 0xcb, 0xdc, 0xed, 0xfe, 0x0f,
	};
	static struct crypto_stress_ctx ctx;
	psa_key_attributes_t shared_attr = PSA_KEY_ATTRIBUTES_INIT;
	size_t base_used = 0U;
	size_t base_blocks = 0U;
	size_t end_used = 0U;
	size_t end_blocks = 0U;
	uint8_t *probe;

	/*
	 * Verify _mbedtls_heap allocations succeed immediately after boot,
	 * proving zephyr_mbedtls_init_threading() ran before init_heap().
	 */
	probe = mbedtls_calloc(1U, 64U);
	zassert_not_null(probe, "boot-time mbedtls_calloc failed");
	mbedtls_free(probe);

	zassert_equal(mbedtls_memory_buffer_alloc_verify(), 0);
	mbedtls_memory_buffer_alloc_cur_get(&base_used, &base_blocks);

	k_sem_reset(&crypto_start_sem);

	psa_set_key_usage_flags(&shared_attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&shared_attr, PSA_ALG_GCM);
	psa_set_key_type(&shared_attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&shared_attr, 128U);
	zassert_equal(psa_import_key(&shared_attr, shared_key_bytes, sizeof(shared_key_bytes),
				     &ctx.shared_key_id),
		      PSA_SUCCESS);

	for (uint32_t i = 0U; i < NUM_WORKERS; i++) {
		ctx.completed_iters[i] = 0U;
		(void)k_thread_create(&worker_threads[i], worker_stacks[i],
				      K_THREAD_STACK_SIZEOF(worker_stacks[i]), crypto_stress_worker,
				      &ctx, UINT_TO_POINTER(i), NULL,
				      K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	}

	for (uint32_t i = 0U; i < NUM_WORKERS; i++) {
		k_sem_give(&crypto_start_sem);
	}

	for (uint32_t i = 0U; i < NUM_WORKERS; i++) {
		zassert_ok(k_thread_join(&worker_threads[i], K_FOREVER));
	}

	for (uint32_t i = 0U; i < NUM_WORKERS; i++) {
		zassert_equal(ctx.completed_iters[i], CRYPTO_ITERATIONS);
	}

	zassert_equal(psa_destroy_key(ctx.shared_key_id), PSA_SUCCESS);

	zassert_equal(mbedtls_memory_buffer_alloc_verify(), 0);

	mbedtls_memory_buffer_alloc_cur_get(&end_used, &end_blocks);
	zassert_equal(end_used, base_used, "mbedtls heap byte leak");
	zassert_equal(end_blocks, base_blocks, "mbedtls heap block leak");
}

ZTEST_SUITE(mbedtls_threading, NULL, NULL, NULL, NULL, NULL);
