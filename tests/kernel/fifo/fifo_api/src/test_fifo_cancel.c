/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fifo.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN 2
/**TESTPOINT: init via K_FIFO_DEFINE*/
K_FIFO_DEFINE(kfifo_c);

struct k_fifo fifo_c;

static K_THREAD_STACK_DEFINE(tstack_cancel, STACK_SIZE);
static struct k_thread thread;

static void t_cancel_wait_entry(void *p1, void *p2, void *p3)
{
	k_sleep(K_MSEC(50));
	k_fifo_cancel_wait((struct k_fifo *)p1);
}

static void tfifo_thread_thread(struct k_fifo *pfifo)
{
	k_tid_t tid = k_thread_create(&thread, tstack_cancel, STACK_SIZE,
				      t_cancel_wait_entry, pfifo, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	uint32_t start_t = k_uptime_get_32();
	void *ret = k_fifo_get(pfifo, K_MSEC(500));
	uint32_t dur = k_uptime_get_32() - start_t;

	/* While we observed the side effect of the last statement
	 * ( call to k_fifo_cancel_wait) of the thread, it's not fact
	 * that it returned, within the thread. Then it may happen
	 * that the test runner below will try to create another
	 * thread in the same stack space, then 1st thread returns
	 * from the call, leading to crash.
	 */
	k_thread_abort(tid);
	zassert_is_null(ret,
			"k_fifo_get didn't get 'timeout expired' status");
	/* 80 includes generous fuzz factor as k_sleep() will add an extra
	 * tick for non-tickless systems, and we may cross another tick
	 * boundary while doing this. We just want to ensure we didn't
	 * hit the timeout anyway.
	 */
	zassert_true(dur < 80,
		     "k_fifo_get didn't get cancelled in expected timeframe");
}

/**
 * @addtogroup kernel_fifo_tests
 * @{
 */

/**
 * @brief Test cancel waiting on a FIFO queue.
 * @details This routine causes first thread pending on fifo (if any),
 * to return from k_fifo_get() with NULL value (as if timeout expired).
 * @see k_fifo_init(),k_fifo_get(), k_fifo_cancel_wait()
 */
ZTEST(fifo_api_1cpu, test_fifo_cancel_wait)
{
	/**TESTPOINT: init via k_fifo_init*/
	k_fifo_init(&fifo_c);
	tfifo_thread_thread(&fifo_c);

	/**TESTPOINT: test K_FIFO_DEFINEed fifo*/
	tfifo_thread_thread(&kfifo_c);
}

/**
 * @}
 */
