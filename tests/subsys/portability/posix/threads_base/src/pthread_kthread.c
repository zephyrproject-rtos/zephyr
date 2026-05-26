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

extern k_tid_t pthread_to_zephyr_thread(pthread_t pth);

#define STACKSIZE 2000
static K_THREAD_STACK_DEFINE(kthread__stack, STACKSIZE);

static pthread_t pthread_thread2;
static struct k_thread *kthread_thread2;

static void *pthread_entry(void *arg)
{
	k_tid_t kself = k_current_get();

	ARG_UNUSED(arg);
	zassert_not_null(kself);

	pthread_t pself = pthread_self();

	zassert_not_equal(0, pself);
	pthread_thread2 = pself;

	k_tid_t kself2 = pthread_to_zephyr_thread(pself);

	zassert_equal_ptr(kself, kself2);
	return NULL;
}

static void kthread_entry(void)
{
	k_tid_t kself = k_current_get();

	zassert_not_null(kself);
	kthread_thread2 = kself;

	pthread_t pself = pthread_self();

	zassert_not_equal(0, pself);

	k_tid_t kself2 = pthread_to_zephyr_thread(pself);

	zassert_equal_ptr(kself, kself2);
}

ZTEST(pthread_kthread, test_pthread_kthread_self)
{
	k_tid_t kself = k_current_get();

	zassert_not_null(kself);

	pthread_t pself = pthread_self();

	zassert_not_equal(0, pself);

	k_tid_t kself2 = pthread_to_zephyr_thread(pself);

	zassert_equal_ptr(kself, kself2);
}

ZTEST(pthread_kthread, test_pthread_kthread_coop)
{
	pthread_t pthread_thread;
	struct k_thread kthread_thread;

	zassert_ok(pthread_create(&pthread_thread, NULL, &pthread_entry, NULL));

	k_thread_create(&kthread_thread, kthread__stack, STACKSIZE, (k_thread_entry_t)kthread_entry,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	zassert_ok(pthread_join(pthread_thread, NULL));
	zassert_equal(pthread_thread, pthread_thread2);

	zassert_ok(k_thread_join(&kthread_thread, K_FOREVER));
	zassert_equal_ptr(&kthread_thread, kthread_thread2);
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(pthread_kthread, NULL, NULL, before, NULL, NULL);
