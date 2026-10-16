/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>

LOG_MODULE_REGISTER(test, LOG_LEVEL_INF);

#define WAIT_TIME K_MSEC(500)
#define BURST_LEN 5

K_SEM_DEFINE(msg_sem, 0, K_SEM_MAX_LIMIT);
K_SEM_DEFINE(done_sem, 0, K_SEM_MAX_LIMIT);
static atomic_t done_cnt;

static void backend_process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	k_sem_give(&msg_sem);
}

static void backend_notify(const struct log_backend *const backend, enum log_backend_evt event,
			   union log_backend_evt_arg *arg)
{
	if (event == LOG_BACKEND_EVT_PROCESS_THREAD_DONE) {
		atomic_inc(&done_cnt);
		k_sem_give(&done_sem);
	}
}

static void backend_panic(const struct log_backend *const backend)
{
}

static const struct log_backend_api backend_api = {
	.process = backend_process,
	.notify = backend_notify,
	.panic = backend_panic,
};

LOG_BACKEND_DEFINE(test_backend, backend_api, true);

/* The test thread is cooperative and the processing thread has the lowest
 * priority, so messages logged back to back are all pending before the
 * processing thread runs.
 */
static void log_and_wait(int cnt)
{
	for (int i = 0; i < cnt; i++) {
		LOG_INF("msg %d", i);
	}

	for (int i = 0; i < cnt; i++) {
		zassert_ok(k_sem_take(&msg_sem, WAIT_TIME), "message %d not processed", i);
	}
}

static void check_one_done_event(void)
{
	zassert_ok(k_sem_take(&done_sem, WAIT_TIME),
		   "PROCESS_THREAD_DONE not sent after processing");
	zassert_equal(k_sem_take(&done_sem, WAIT_TIME), -EAGAIN,
		      "unexpected extra PROCESS_THREAD_DONE");
	zassert_equal(atomic_get(&done_cnt), 1);
}

ZTEST(log_backend_events, test_no_message)
{
	zassert_equal(k_sem_take(&done_sem, WAIT_TIME), -EAGAIN,
		      "PROCESS_THREAD_DONE sent without any message");
	zassert_equal(atomic_get(&done_cnt), 0);
}

ZTEST(log_backend_events, test_single_message)
{
	log_and_wait(1);
	check_one_done_event();
}

ZTEST(log_backend_events, test_burst)
{
	log_and_wait(BURST_LEN);
	check_one_done_event();
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&msg_sem);
	k_sem_reset(&done_sem);
	atomic_clear(&done_cnt);
}

ZTEST_SUITE(log_backend_events, NULL, NULL, before, NULL, NULL);
