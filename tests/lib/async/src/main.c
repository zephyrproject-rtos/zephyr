/*
 * Copyright (c) 2022 Intel Corporation+
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/async.h>
#include <zephyr/kernel.h>
#include <ztest.h>

K_SEM_DEFINE(sem, 0, 1);
static struct async_sem asem = {
	.sem = &sem
};

void test_async_sem(void)
{
	int res;

	async_semaphore_cb(NULL, 1, &asem);
	res = k_sem_take(&sem, K_NO_WAIT);
	zassert_ok(res, "expected sem take success");
	zassert_equal(asem.result, 1, "expected result");
}


static struct k_poll_signal sig;

void test_async_signal(void)
{
	unsigned int signaled;
	int result;

	k_poll_signal_init(&sig);
	async_signal_cb(NULL, 2, &sig);
	k_poll_signal_check(&sig, &signaled, &result);

	zassert_not_equal(signaled, 0, "expected signal");
	zassert_equal(result, 2, "expected result");
}

K_MUTEX_DEFINE(mutex);
static struct async_mutex amutex = {
	.mutex = &mutex
};

void test_async_mutex(void)
{
	int res;

	res = k_mutex_lock(&mutex, K_NO_WAIT);
	zassert_ok(res, "expected mutex lock");
	async_mutex_cb(NULL, 3, &amutex);
	res = k_mutex_lock(amutex.mutex, K_NO_WAIT);
	zassert_ok(res, "expected mutex lock");
	zassert_equal(amutex.result, 3, "expected result");
}

K_SEM_DEFINE(work_done, 0, 1);
void test_async_work_cb(struct k_work *work);
K_WORK_DEFINE(work, test_async_work_cb);
static struct async_work awork = {
	.work = &work,
};


void test_async_work_cb(struct k_work *work)
{
	zassert_equal(awork.result, 4, "expected result");

	k_sem_give(&work_done);
}

void test_async_work(void)
{
	int res;

	async_work_cb(NULL, 4, &awork);
	res = k_sem_take(&work_done, K_MSEC(1));
	zassert_ok(res, "expected work done sem take");
}

void test_main(void)
{
	ztest_test_suite(test_async,
			 ztest_unit_test(test_async_sem),
			 ztest_unit_test(test_async_signal),
			 ztest_unit_test(test_async_mutex),
			 ztest_unit_test(test_async_work)
			 );
	ztest_run_test_suite(test_async);
}
