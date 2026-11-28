/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <mbedtls/memory_buffer_alloc.h>
#include <mbedtls/platform.h>
#include <mbedtls/threading.h>
#include <psa/crypto.h>
#include <zephyr/ztest.h>

#if !defined(MBEDTLS_THREADING_C) || !defined(MBEDTLS_THREADING_PTHREAD)
#error "The PSA concurrency test requires the Mbed TLS pthread backend"
#endif

#define WORKER_COUNT 4U
#define ITERATIONS 64U
#define BLOCK_SIZE PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES)

static K_SEM_DEFINE(keys_held, 0, WORKER_COUNT);
static K_SEM_DEFINE(release_workers, 0, WORKER_COUNT);
static K_THREAD_STACK_ARRAY_DEFINE(stacks, WORKER_COUNT, CONFIG_ZTEST_STACK_SIZE);
static struct k_thread workers[WORKER_COUNT];

static void crypto_worker(void *id, void *unused1, void *unused2)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t key[BLOCK_SIZE];
	uint8_t encrypted[BLOCK_SIZE];
	uint8_t decrypted[BLOCK_SIZE];
	size_t len;
	int worker = POINTER_TO_INT(id);

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT |
				     PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECB_NO_PADDING);
	for (size_t i = 0; i < ITERATIONS; i++) {
		psa_key_id_t key_id = PSA_KEY_ID_NULL;
		uint8_t *buffer = mbedtls_calloc(1U, BLOCK_SIZE);

		zassert_not_null(buffer);
		zassert_equal(psa_generate_random(key, sizeof(key)), PSA_SUCCESS);
		memset(buffer, i + worker, BLOCK_SIZE);
		zassert_equal(psa_import_key(&attr, key, sizeof(key), &key_id), PSA_SUCCESS);
		if (i == 0U) {
			k_sem_give(&keys_held);
			zassert_ok(k_sem_take(&release_workers, K_SECONDS(5)));
		}
		k_yield();
		zassert_equal(psa_export_key(key_id, decrypted, sizeof(decrypted), &len),
			      PSA_SUCCESS);
		zassert_equal(len, sizeof(key));
		zassert_mem_equal(decrypted, key, sizeof(key));
		zassert_equal(psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, buffer, BLOCK_SIZE,
						 encrypted, sizeof(encrypted), &len), PSA_SUCCESS);
		zassert_equal(len, sizeof(encrypted));
		k_yield();
		zassert_equal(psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, encrypted,
						 sizeof(encrypted), decrypted, sizeof(decrypted),
						 &len), PSA_SUCCESS);
		zassert_equal(len, BLOCK_SIZE);
		for (size_t j = 0; j < BLOCK_SIZE; j++) {
			zassert_equal(decrypted[j], (uint8_t)(i + worker));
		}
		zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS);
		mbedtls_free(buffer);
	}
}

ZTEST(psa_crypto_threading, test_concurrent_provider_and_heap)
{
	for (size_t i = 0; i < WORKER_COUNT; i++) {
		k_thread_create(&workers[i], stacks[i], K_THREAD_STACK_SIZEOF(stacks[i]),
				crypto_worker, UINT_TO_POINTER(i), NULL, NULL, K_PRIO_PREEMPT(0), 0,
				K_NO_WAIT);
	}
	for (size_t i = 0; i < WORKER_COUNT; i++) {
		zassert_ok(k_sem_take(&keys_held, K_SECONDS(5)));
	}
	zassert_ok(mbedtls_memory_buffer_alloc_verify());
	for (size_t i = 0; i < WORKER_COUNT; i++) {
		k_sem_give(&release_workers);
	}
	for (size_t i = 0; i < WORKER_COUNT; i++) {
		zassert_ok(k_thread_join(&workers[i], K_SECONDS(5)));
	}
	zassert_ok(mbedtls_memory_buffer_alloc_verify());
}

struct signal_state {
	mbedtls_threading_mutex_t lock;
	mbedtls_threading_condition_variable_t condition;
	struct k_sem ready;
	bool resume;
};

static void wait_for_signal(void *arg, void *unused1, void *unused2)
{
	struct signal_state *state = arg;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	zassert_ok(mbedtls_mutex_lock(&state->lock));
	k_sem_give(&state->ready);
	while (!state->resume) {
		zassert_ok(mbedtls_condition_variable_wait(&state->condition, &state->lock));
	}
	zassert_ok(mbedtls_mutex_unlock(&state->lock));
}

ZTEST(psa_crypto_threading, test_condition_variable)
{
	struct signal_state state = {0};

	k_sem_init(&state.ready, 0, 1);
	mbedtls_mutex_init(&state.lock);
	zassert_ok(mbedtls_condition_variable_init(&state.condition));
	zassert_ok(mbedtls_condition_variable_signal(&state.condition));
	k_thread_create(&workers[0], stacks[0], K_THREAD_STACK_SIZEOF(stacks[0]), wait_for_signal,
			&state, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&state.ready, K_SECONDS(5)));
	zassert_ok(mbedtls_mutex_lock(&state.lock));
	state.resume = true;
	zassert_ok(mbedtls_condition_variable_broadcast(&state.condition));
	zassert_ok(mbedtls_mutex_unlock(&state.lock));
	zassert_ok(k_thread_join(&workers[0], K_SECONDS(5)));
	mbedtls_condition_variable_free(&state.condition);
	mbedtls_mutex_free(&state.lock);
}

ZTEST_SUITE(psa_crypto_threading, NULL, NULL, NULL, NULL, NULL);
