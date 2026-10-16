/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/ztest.h>

/* channel reload period, long enough to never time out during a test */
#define TASK_WDT_PERIOD_MS 2000U
#define DEADLINE_MIN_MS    100U
#define DEADLINE_MAX_MS    300U
#define TASK_WDT_WAIT_MS   600U

/* START -> WORK -> END -> START, with a deadline from START to END */
enum {
	CP_START,
	CP_WORK,
	CP_END,
};

static const uint32_t next[] = {
	[CP_START] = BIT(CP_WORK),
	[CP_WORK] = BIT(CP_WORK) | BIT(CP_END),
	[CP_END] = BIT(CP_START),
};

static const struct task_wdt_deadline deadlines[] = {
	{.start = CP_START, .end = CP_END, .min_ms = DEADLINE_MIN_MS, .max_ms = DEADLINE_MAX_MS},
};

TASK_WDT_SUPERVISION_DEFINE(loop, next, deadlines);

/* a second entity, to check that a channel reports for one entity only */
TASK_WDT_SUPERVISION_DEFINE(other, next, deadlines);

static const struct task_wdt_deadline bad_deadlines[] = {
	{.start = CP_START, .end = CP_END, .min_ms = DEADLINE_MAX_MS, .max_ms = DEADLINE_MIN_MS},
};

TASK_WDT_SUPERVISION_DEFINE(bad, next, bad_deadlines);

#ifdef CONFIG_TASK_WDT_SUPERVISION

static K_SEM_DEFINE(fault_sem, 0, 10);
static atomic_t fault_count;
static atomic_t fault_reason;
static atomic_t fault_channel;

static void fault_callback(const struct task_wdt_fault *fault, void *user_data)
{
	ARG_UNUSED(user_data);

	atomic_inc(&fault_count);
	atomic_set(&fault_reason, fault->reason);
	atomic_set(&fault_channel, fault->channel_id);
	k_sem_give(&fault_sem);
}

static int start_loop(void)
{
	int channel_id = task_wdt_add_ext(TASK_WDT_PERIOD_MS, fault_callback, NULL);

	zassert_true(channel_id >= 0, "task_wdt_add_ext() failed: %d", channel_id);
	zassert_ok(task_wdt_supervision_start(&loop, channel_id));

	return channel_id;
}

static void check_fault(int channel_id, enum task_wdt_fault_reason reason)
{
	zassert_ok(k_sem_take(&fault_sem, K_MSEC(TASK_WDT_WAIT_MS)), "no fault reported");
	zassert_equal(atomic_get(&fault_count), 1);
	zassert_equal(atomic_get(&fault_reason), reason);
	zassert_equal(atomic_get(&fault_channel), channel_id);

	/* the fault was checked, only count the next ones */
	atomic_set(&fault_count, 0);
}

static void check_no_fault(void)
{
	zassert_equal(k_sem_take(&fault_sem, K_MSEC(TASK_WDT_WAIT_MS)), -EAGAIN,
		      "unexpected fault %ld", atomic_get(&fault_reason));
	zassert_equal(atomic_get(&fault_count), 0);
}

ZTEST(task_wdt_supervision, test_valid_flow)
{
	start_loop();

	for (int i = 0; i < 3; i++) {
		zassert_ok(task_wdt_checkpoint(&loop, CP_START));
		zassert_ok(task_wdt_checkpoint(&loop, CP_WORK));
		k_sleep(K_MSEC((DEADLINE_MIN_MS + DEADLINE_MAX_MS) / 2));
		zassert_ok(task_wdt_checkpoint(&loop, CP_WORK));
		zassert_ok(task_wdt_checkpoint(&loop, CP_END));
	}

	/* the deadline ended, so waiting past its maximum is fine */
	check_no_fault();
}

ZTEST(task_wdt_supervision, test_first_checkpoint)
{
	start_loop();

	/* any checkpoint may be reached first */
	zassert_ok(task_wdt_checkpoint(&loop, CP_END));
	zassert_ok(task_wdt_checkpoint(&loop, CP_START));
	k_sleep(K_MSEC(DEADLINE_MIN_MS * 2));
	zassert_ok(task_wdt_checkpoint(&loop, CP_WORK));
	zassert_ok(task_wdt_checkpoint(&loop, CP_END));

	check_no_fault();
}

ZTEST(task_wdt_supervision, test_sequence)
{
	int channel_id = start_loop();

	zassert_ok(task_wdt_checkpoint(&loop, CP_START));
	k_sleep(K_MSEC(DEADLINE_MIN_MS * 2));

	/* WORK was skipped, reported before task_wdt_checkpoint() returns */
	zassert_ok(task_wdt_checkpoint(&loop, CP_END));
	zassert_equal(atomic_get(&fault_count), 1);
	check_fault(channel_id, TASK_WDT_FAULT_SEQUENCE);

	/* the deadline ended with the fault */
	check_no_fault();
}

ZTEST(task_wdt_supervision, test_deadline_min)
{
	int channel_id = start_loop();

	zassert_ok(task_wdt_checkpoint(&loop, CP_START));
	zassert_ok(task_wdt_checkpoint(&loop, CP_WORK));
	zassert_ok(task_wdt_checkpoint(&loop, CP_END));
	zassert_equal(atomic_get(&fault_count), 1);
	check_fault(channel_id, TASK_WDT_FAULT_DEADLINE_MIN);

	check_no_fault();
}

ZTEST(task_wdt_supervision, test_deadline_max)
{
	int channel_id = start_loop();
	int64_t start = k_uptime_get();

	zassert_ok(task_wdt_checkpoint(&loop, CP_START));
	zassert_ok(task_wdt_checkpoint(&loop, CP_WORK));

	/* feeding the channel does not extend the deadline */
	for (int i = 0; i < 4; i++) {
		k_sleep(K_MSEC(DEADLINE_MAX_MS / 4));
		zassert_ok(task_wdt_feed(channel_id));
	}

	check_fault(channel_id, TASK_WDT_FAULT_DEADLINE_MAX);
	zassert_true(k_uptime_get() - start >= DEADLINE_MAX_MS, "deadline expired early");

	/* reported once, and the channel itself is still supervised */
	check_no_fault();
	zassert_ok(task_wdt_feed(channel_id));

	/* the next cycle is supervised again */
	zassert_ok(task_wdt_checkpoint(&loop, CP_END));
	zassert_ok(task_wdt_checkpoint(&loop, CP_START));
	check_fault(channel_id, TASK_WDT_FAULT_DEADLINE_MAX);
}

ZTEST(task_wdt_supervision, test_channel_timeout)
{
	int channel_id = task_wdt_add_ext(DEADLINE_MAX_MS, fault_callback, NULL);

	zassert_true(channel_id >= 0, "task_wdt_add_ext() failed: %d", channel_id);
	zassert_ok(task_wdt_supervision_start(&loop, channel_id));

	/* without a running deadline, the channel times out as usual */
	check_fault(channel_id, TASK_WDT_FAULT_TIMEOUT);
}

ZTEST(task_wdt_supervision, test_stop)
{
	start_loop();

	zassert_ok(task_wdt_checkpoint(&loop, CP_START));
	zassert_ok(task_wdt_supervision_stop(&loop));

	/* the running deadline was cancelled */
	check_no_fault();

	zassert_equal(task_wdt_supervision_stop(&loop), -EINVAL);
	zassert_equal(task_wdt_checkpoint(&loop, CP_START), -EINVAL);
}

ZTEST(task_wdt_supervision, test_delete_channel)
{
	int channel_id = start_loop();

	zassert_ok(task_wdt_checkpoint(&loop, CP_START));
	zassert_ok(task_wdt_delete(channel_id));

	/* deleting the channel stops the supervision */
	check_no_fault();
	zassert_equal(task_wdt_checkpoint(&loop, CP_WORK), -EINVAL);
	zassert_equal(task_wdt_supervision_stop(&loop), -EINVAL);
}

ZTEST(task_wdt_supervision, test_suspend_resume)
{
	start_loop();

	zassert_ok(task_wdt_checkpoint(&loop, CP_START));
	task_wdt_suspend();
	k_sleep(K_MSEC(TASK_WDT_WAIT_MS));
	task_wdt_resume();

	/* resuming cancels the running deadline, the next cycle is supervised */
	check_no_fault();
	zassert_ok(task_wdt_checkpoint(&loop, CP_WORK));
	zassert_ok(task_wdt_checkpoint(&loop, CP_END));
	check_no_fault();
}

ZTEST(task_wdt_supervision, test_invalid)
{
	int channel_id = start_loop();

	zassert_equal(task_wdt_supervision_start(NULL, channel_id), -EINVAL);
	zassert_equal(task_wdt_supervision_start(&other, -1), -EINVAL);
	zassert_equal(task_wdt_supervision_start(&other, CONFIG_TASK_WDT_CHANNELS), -EINVAL);
	zassert_equal(task_wdt_supervision_start(&other, channel_id + 1), -EINVAL);
	zassert_equal(task_wdt_supervision_start(&bad, channel_id), -EINVAL);

	zassert_equal(task_wdt_supervision_start(&loop, channel_id), -EBUSY);
	zassert_equal(task_wdt_supervision_start(&other, channel_id), -EBUSY);

	zassert_equal(task_wdt_supervision_stop(NULL), -EINVAL);
	zassert_equal(task_wdt_supervision_stop(&other), -EINVAL);

	zassert_equal(task_wdt_checkpoint(NULL, CP_START), -EINVAL);
	zassert_equal(task_wdt_checkpoint(&loop, ARRAY_SIZE(next)), -EINVAL);
	zassert_equal(task_wdt_checkpoint(&other, CP_START), -EINVAL);

	check_no_fault();
}

static void task_wdt_supervision_before(void *arg)
{
	ARG_UNUSED(arg);

	k_sem_reset(&fault_sem);
	atomic_set(&fault_count, 0);
	atomic_set(&fault_reason, -1);
	atomic_set(&fault_channel, -1);
}

static void task_wdt_supervision_after(void *arg)
{
	ARG_UNUSED(arg);

	task_wdt_supervision_stop(&loop);
	task_wdt_supervision_stop(&other);

	for (int channel_id = 0; channel_id < CONFIG_TASK_WDT_CHANNELS; channel_id++) {
		task_wdt_delete(channel_id);
	}
}

#else /* CONFIG_TASK_WDT_SUPERVISION */

ZTEST(task_wdt_supervision, test_disabled)
{
	int channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, NULL, NULL);

	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);
	zassert_equal(task_wdt_supervision_start(&loop, channel_id), -ENOTSUP);
	zassert_equal(task_wdt_supervision_stop(&loop), -ENOTSUP);
	zassert_equal(task_wdt_checkpoint(&loop, CP_START), -ENOTSUP);
	zassert_ok(task_wdt_delete(channel_id));
}

#define task_wdt_supervision_before NULL
#define task_wdt_supervision_after  NULL

#endif /* CONFIG_TASK_WDT_SUPERVISION */

static void *task_wdt_supervision_setup(void)
{
	zassert_ok(task_wdt_init(NULL));

	return NULL;
}

ZTEST_SUITE(task_wdt_supervision, NULL, task_wdt_supervision_setup, task_wdt_supervision_before,
	    task_wdt_supervision_after, NULL);
