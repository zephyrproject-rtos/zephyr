/*
 * Copyright (c) 2018-2023 Intel Corporation
 * Copyright (c) 2026 Göthel Software
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define STACKSIZE 2000
static K_THREAD_STACK_DEFINE(kthread__stack, STACKSIZE);

static pthread_key_t key;
static uint8_t const tagP = 0x11U;
static uint8_t const tagK = 0x22U;
static uint8_t const tagM = 0x22U;

static void *pthread_entry(void *arg)
{
	pthread_t pself = pthread_self();

	ARG_UNUSED(arg);
	zassert_not_equal(0, pself);

	uint8_t *val = (uint8_t *)pthread_getspecific(key);

	zassert_is_null(val);

	zassert_ok(pthread_setspecific(key, (void *)&tagP));

	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagP);
	zassert_equal(*val, tagP);
	return NULL;
}

static void kthread_entry(void)
{
	pthread_t pself = pthread_self();

	zassert_not_equal(0, pself);

	uint8_t *val = (uint8_t *)pthread_getspecific(key);

	zassert_is_null(val);

	zassert_ok(pthread_setspecific(key, (void *)&tagK));

	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagK);
	zassert_equal(*val, tagK);
}

ZTEST(pthread_specific, test_pthread_specific_pkthread)
{
	pthread_t pthread_thread;
	struct k_thread kthread_thread;

	zassert_ok(pthread_key_create(&key, NULL));

	uint8_t *val = (uint8_t *)pthread_getspecific(key);

	zassert_is_null(val);

	zassert_ok(pthread_setspecific(key, (void *)&tagM));

	val = (uint8_t *)pthread_getspecific(key);
	zassert_not_null(val);
	zassert_equal_ptr(val, &tagM);
	zassert_equal(*val, tagM);
	{
		zassert_ok(pthread_create(&pthread_thread, NULL, &pthread_entry, NULL));

		k_thread_create(&kthread_thread, kthread__stack, STACKSIZE,
				(k_thread_entry_t)kthread_entry, NULL, NULL, NULL, K_PRIO_COOP(7),
				0, K_NO_WAIT);

		zassert_ok(pthread_join(pthread_thread, NULL));
		zassert_ok(k_thread_join(&kthread_thread, K_FOREVER));
	}
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(pthread_specific, NULL, NULL, before, NULL, NULL);
