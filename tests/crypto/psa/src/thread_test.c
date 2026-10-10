/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <mbedtls/threading.h>

/*
 * Mutex test sequence dumped from TF PSA Crypt test environment
 */

ZTEST_USER(psa_crypto_test_suite, test_mbedtls_mutex_nominal)
{
	mbedtls_threading_mutex_t mutex;

	mbedtls_mutex_init(&mutex);

	for (int i = 0; i < 5; i++) {
		zassert_ok(mbedtls_mutex_lock(&mutex));
		zassert_ok(mbedtls_mutex_unlock(&mutex), 0);
	}

	mbedtls_mutex_free(&mutex);
}

ZTEST_USER(psa_crypto_test_suite, test_mbedtls_mutex_no_init)
{
	mbedtls_threading_mutex_t mutex;

	memset(&mutex, 0, sizeof(mutex));
	zassert_equal(mbedtls_mutex_lock(&mutex), MBEDTLS_ERR_THREADING_USAGE_ERROR);
	zassert_equal(mbedtls_mutex_unlock(&mutex), MBEDTLS_ERR_THREADING_USAGE_ERROR);
	mbedtls_mutex_free(&mutex);
}

ZTEST_USER(psa_crypto_test_suite, test_mbedtls_mutex_unbalanced_unlock)
{
	mbedtls_threading_mutex_t mutex;

	mbedtls_mutex_init(&mutex);
	mbedtls_mutex_free(&mutex);
	zassert_equal(mbedtls_mutex_lock(&mutex), MBEDTLS_ERR_THREADING_USAGE_ERROR);
	zassert_equal(mbedtls_mutex_unlock(&mutex), MBEDTLS_ERR_THREADING_USAGE_ERROR);
	mbedtls_mutex_free(&mutex);
}

ZTEST_USER(psa_crypto_test_suite, test_mbedtls_condvar_nominal)
{
	mbedtls_threading_condition_variable_t cond;

	mbedtls_condition_variable_init(&cond);
	zassert_ok(mbedtls_condition_variable_signal(&cond));
	zassert_ok(mbedtls_condition_variable_broadcast(&cond));
	mbedtls_condition_variable_free(&cond);
}

/*
 * Test mutex and condvar using the ztest thread and a conpanion test thread,
 * synchornised using mbedtls mutex and condvar resources.
 */

static mbedtls_threading_condition_variable_t test_thread_condvar;
static mbedtls_threading_mutex_t test_thread_mutex;
K_THREAD_STACK_DEFINE(test_thread_stack_area, 512);
static struct k_thread test_thread;
static int test_thread_step;

void test_thread_func(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Active waiting for main thread to reach step 1 */
	while (test_thread_step == 0) {
		k_yield();
	}

	/* Lock mutex to move from step 1 to step 2 */
	mbedtls_mutex_lock(&test_thread_mutex);
	zassert_equal(test_thread_step, 1);
	test_thread_step = 2;

	/* Yielded waiting for main loop to reach step 3 */
	zassert_ok(mbedtls_condition_variable_wait(&test_thread_condvar, &test_thread_mutex));
	zassert_equal(test_thread_step, 3);
	test_thread_step = 4;

	/* Yielded waiting for main loop to reach step 5 */
	zassert_ok(mbedtls_condition_variable_wait(&test_thread_condvar, &test_thread_mutex));
	zassert_equal(test_thread_step, 5);
	test_thread_step = 6;

	mbedtls_mutex_unlock(&test_thread_mutex);
}

ZTEST_USER(psa_crypto_test_suite, test_mbedtls_sync_on_mutex_condvar)
{
	mbedtls_mutex_init(&test_thread_mutex);
	mbedtls_condition_variable_init(&test_thread_condvar);

	k_thread_create(&test_thread, test_thread_stack_area,
			K_THREAD_STACK_SIZEOF(test_thread_stack_area),
			test_thread_func, NULL, NULL, NULL, 10, 0,
			K_FOREVER);
	k_thread_start(&test_thread);

	/*
	 * Main thread(test_main) priority was 10 but ztest thread runs at
	 * priority -1. To run the test smoothly make both main and ztest
	 * threads run at same priority level.
	 */
	k_thread_priority_set(k_current_get(), 10);

	/* Enter test step 1, check test thread reaches step 2 using mutex */
	mbedtls_mutex_lock(&test_thread_mutex);
	zassert_equal(test_thread_step, 0);
	test_thread_step = 1;
	/* Let other thread to opportinuty to execute: test thread shall still wait on mutex */
	k_yield();
	zassert_equal(test_thread_step, 1);
	mbedtls_mutex_unlock(&test_thread_mutex);
	/* Let test thread the opportunity to get the mutex and reach step 2 */
	k_yield();
	zassert_equal(test_thread_step, 2);

	/* Enter test step 3, check test thread reaches step 4 using condvar signal */
	mbedtls_mutex_lock(&test_thread_mutex);
	zassert_equal(test_thread_step, 2);
	test_thread_step = 3;
	mbedtls_mutex_unlock(&test_thread_mutex);
	/* Let other thread to opportinuty to execute: test thread shall still wait on condvar */
	k_yield();
	zassert_equal(test_thread_step, 3);
	zassert_ok(mbedtls_condition_variable_signal(&test_thread_condvar));
	/* Let test thread the opportunity to get the mutex and reach step 4 */
	k_yield();
	zassert_equal(test_thread_step, 4);

	/* Enter test step 5, check test thread reaches step 6 using condvar broadcast */
	mbedtls_mutex_lock(&test_thread_mutex);
	zassert_equal(test_thread_step, 4);
	test_thread_step = 5;
	mbedtls_mutex_unlock(&test_thread_mutex);
	/* Let other thread to opportinuty to execute: test thread shall still wait on condvar */
	k_yield();
	zassert_equal(test_thread_step, 5);
	zassert_ok(mbedtls_condition_variable_broadcast(&test_thread_condvar));
	/* Let test thread the opportunity to get the mutex and reach step 6 */
	k_yield();
	zassert_equal(test_thread_step, 6);

	k_thread_abort(&test_thread);
}
