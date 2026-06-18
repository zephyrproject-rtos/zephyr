/* main.c - wpa_supplicant command workqueue offload unit tests */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include "supp_cmd_wq.h"

/* Dedicated command workqueue, provided to the code under test through the
 * get_cmd_wq() hook that supp_main.c normally implements.
 */
#define CMD_WQ_STACK_SIZE 2048
static K_THREAD_STACK_DEFINE(cmd_wq_stack, CMD_WQ_STACK_SIZE);
static struct k_work_q cmd_wq;

struct k_work_q *get_cmd_wq(void)
{
	return &cmd_wq;
}

/* Typed operation context, mirroring how the real supplicant ops embed
 * struct supplicant_cmd and recover their context with CONTAINER_OF().
 */
struct test_op {
	struct supplicant_cmd cmd;
	int in;
	int out;
	k_tid_t ran_on;
};

static atomic_t calls;
static atomic_t nested_calls;
static k_tid_t nested_ran_on;

static void echo_handler(struct k_work *work)
{
	struct test_op *op = CONTAINER_OF(work, struct test_op, cmd.work);

	op->ran_on = k_current_get();
	op->out = op->in;
	atomic_inc(&calls);
}

static void nested_handler(struct k_work *work)
{
	struct test_op *op = CONTAINER_OF(work, struct test_op, cmd.work);

	op->ran_on = k_current_get();
	atomic_inc(&nested_calls);
}

static void reentrant_handler(struct k_work *work)
{
	struct test_op *op = CONTAINER_OF(work, struct test_op, cmd.work);
	struct test_op nested = {0};

	op->ran_on = k_current_get();
	atomic_inc(&calls);

	/* Issue a nested request from within the workqueue context. */
	supplicant_run_on_cmd_wq(&nested.cmd, nested_handler);
	nested_ran_on = nested.ran_on;
}

static void reset_state(void)
{
	atomic_clear(&calls);
	atomic_clear(&nested_calls);
	nested_ran_on = NULL;
}

/* The handler runs on the command workqueue thread, not the caller, exactly
 * once.
 */
ZTEST(hostap_cmd_wq, test_runs_on_workqueue)
{
	struct test_op op = { .in = 42 };

	reset_state();

	supplicant_run_on_cmd_wq(&op.cmd, echo_handler);

	zassert_equal(atomic_get(&calls), 1, "handler should run exactly once");
	zassert_not_equal(op.ran_on, k_current_get(),
			  "handler must run on the command workqueue, not the caller");
	zassert_equal(op.ran_on, &cmd_wq.thread,
		      "handler must run on the command workqueue thread");
}

/* A result stored by the handler in the caller-owned context is visible once
 * supplicant_run_on_cmd_wq() returns.
 */
ZTEST(hostap_cmd_wq, test_result_propagation)
{
	struct test_op op = { .in = -EIO };

	reset_state();

	supplicant_run_on_cmd_wq(&op.cmd, echo_handler);

	zassert_equal(op.out, -EIO, "result stored by the handler must reach the caller");
	zassert_equal(atomic_get(&calls), 1, "handler should run exactly once");
}

/* A nested request issued from within a handler (i.e. from the workqueue
 * thread) is handled inline by the re-entrancy guard - it runs on the same
 * thread and does not dead-lock the single-threaded queue.
 */
ZTEST(hostap_cmd_wq, test_reentrant_runs_inline)
{
	struct test_op op = {0};

	reset_state();

	supplicant_run_on_cmd_wq(&op.cmd, reentrant_handler);

	zassert_equal(atomic_get(&calls), 1, "outer handler should run once");
	zassert_equal(atomic_get(&nested_calls), 1, "nested handler should run once");
	zassert_equal(nested_ran_on, op.ran_on,
		      "nested op must run inline on the workqueue thread");
}

static void *setup(void)
{
	static const struct k_work_queue_config cfg = {
		.name = "test_cmd_wq",
	};

	k_work_queue_init(&cmd_wq);
	k_work_queue_start(&cmd_wq, cmd_wq_stack, K_THREAD_STACK_SIZEOF(cmd_wq_stack),
			   K_PRIO_PREEMPT(5), &cfg);

	return NULL;
}

ZTEST_SUITE(hostap_cmd_wq, NULL, setup, NULL, NULL, NULL);
