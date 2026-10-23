/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/ztest.h>

#define TASK_WDT_PERIOD_MS   100U
#define TASK_WDT_WAIT_MS     1000U
#define ESCALATION_PERIOD_MS 2000U

#ifdef CONFIG_TASK_WDT_GROUPS

static K_SEM_DEFINE(callback_sem, 0, 10);
static K_SEM_DEFINE(recovery_sem, 0, 10);
static atomic_t callback_count;
static atomic_t recovery_count;
static atomic_t recovery_in_isr;
static atomic_t recovery_reason;
static atomic_t recovery_feed;
static char recovery_thread_name[32];

static void fault_callback(const struct task_wdt_fault *fault, void *user_data)
{
	ARG_UNUSED(fault);
	ARG_UNUSED(user_data);

	atomic_inc(&callback_count);
	k_sem_give(&callback_sem);
}

static void recovery_fn(const struct task_wdt_fault *fault, void *user_data)
{
	const char *name = k_thread_name_get(k_current_get());

	zassert_equal_ptr(user_data, &recovery_count);

	atomic_set(&recovery_in_isr, k_is_in_isr());
	atomic_set(&recovery_reason, fault->reason);
	strncpy(recovery_thread_name, name != NULL ? name : "", sizeof(recovery_thread_name) - 1);

	/* a real application would restart the faulty thread here */
	if (atomic_get(&recovery_feed)) {
		zassert_ok(task_wdt_feed(fault->channel_id));
	}

	atomic_inc(&recovery_count);
	k_sem_give(&recovery_sem);
}

static const struct task_wdt_escalation escalate_twice = {
	.faults = 2U,
	.period_ms = ESCALATION_PERIOD_MS,
};

static const struct task_wdt_escalation escalate_short = {
	.faults = 2U,
	.period_ms = TASK_WDT_PERIOD_MS,
};

TASK_WDT_GROUP_DEFINE(callback_group, TASK_WDT_ACTION_CALLBACK, recovery_fn, NULL);
TASK_WDT_GROUP_DEFINE(recover_group, TASK_WDT_ACTION_RECOVER, recovery_fn, NULL);
TASK_WDT_GROUP_DEFINE(escalating_group, TASK_WDT_ACTION_CALLBACK, recovery_fn, &escalate_twice);
TASK_WDT_GROUP_DEFINE(short_group, TASK_WDT_ACTION_CALLBACK, recovery_fn, &escalate_short);

static int add_channel(struct task_wdt_group *group)
{
	int channel_id = task_wdt_add_ext(TASK_WDT_PERIOD_MS, fault_callback, &recovery_count);

	zassert_true(channel_id >= 0, "task_wdt_add_ext() failed: %d", channel_id);
	zassert_ok(task_wdt_channel_group_set(channel_id, group));

	return channel_id;
}

static void wait_callback(void)
{
	zassert_ok(k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS)), "no callback");
}

static void wait_recovery(void)
{
	zassert_ok(k_sem_take(&recovery_sem, K_MSEC(TASK_WDT_WAIT_MS)), "no recovery");
}

ZTEST(task_wdt_group, test_callback)
{
	int channel_id = add_channel(&callback_group);

	wait_callback();
	zassert_equal(atomic_get(&recovery_count), 0);
	zassert_equal(task_wdt_group_action_get(&callback_group), TASK_WDT_ACTION_CALLBACK);

	/* once the channel was fed again, the next fault is handled the same way */
	zassert_ok(task_wdt_feed(channel_id));
	wait_callback();
	zassert_equal(atomic_get(&recovery_count), 0);
}

ZTEST(task_wdt_group, test_recover)
{
	add_channel(&recover_group);

	/* the recovery runs in thread context, on the recovery work queue */
	wait_recovery();
	zassert_equal(atomic_get(&callback_count), 0);
	zassert_false(atomic_get(&recovery_in_isr), "recovery called from an ISR");
	zassert_equal(atomic_get(&recovery_reason), TASK_WDT_FAULT_TIMEOUT);
	zassert_str_equal(recovery_thread_name, "task_wdt_recovery");

	/* the recovery fed the channel, so the next timeout is recovered again */
	wait_recovery();
	zassert_equal(atomic_get(&callback_count), 0);
	zassert_equal(task_wdt_group_action_get(&recover_group), TASK_WDT_ACTION_RECOVER);
}

ZTEST(task_wdt_group, test_feed_ends_recovery)
{
	int channel_id = add_channel(&recover_group);

	/* the recovery does not feed the channel, the test does it instead */
	atomic_set(&recovery_feed, 0);
	wait_recovery();
	zassert_ok(task_wdt_feed(channel_id));

	/* feeding the channel ended the recovery, so the next timeout is recovered too */
	wait_recovery();
	zassert_ok(task_wdt_delete(channel_id));
}

ZTEST(task_wdt_group, test_escalation)
{
	int channel_id = add_channel(&escalating_group);

	/* the first fault calls the channel callback */
	wait_callback();
	zassert_equal(atomic_get(&recovery_count), 0);
	zassert_equal(task_wdt_group_action_get(&escalating_group), TASK_WDT_ACTION_CALLBACK);

	/* the second fault within the period escalates to the recovery */
	zassert_ok(task_wdt_feed(channel_id));
	wait_recovery();
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(task_wdt_group_action_get(&escalating_group), TASK_WDT_ACTION_RECOVER);

	/* going back to the initial action */
	zassert_ok(task_wdt_group_reset(&escalating_group));
	zassert_equal(task_wdt_group_action_get(&escalating_group), TASK_WDT_ACTION_CALLBACK);
	zassert_ok(task_wdt_feed(channel_id));
	wait_callback();
	zassert_equal(atomic_get(&recovery_count), 1);
	zassert_equal(task_wdt_group_action_get(&escalating_group), TASK_WDT_ACTION_CALLBACK);
}

ZTEST(task_wdt_group, test_escalation_shared)
{
	int first = add_channel(&escalating_group);
	int second = add_channel(&escalating_group);

	/* the faults of all channels of the group are counted together */
	wait_callback();
	zassert_ok(task_wdt_feed(first));
	wait_recovery();
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(task_wdt_group_action_get(&escalating_group), TASK_WDT_ACTION_RECOVER);

	ARG_UNUSED(second);
}

ZTEST(task_wdt_group, test_escalation_period)
{
	int channel_id = add_channel(&short_group);

	/* faults further apart than the period do not escalate */
	for (int i = 0; i < 3; i++) {
		wait_callback();
		k_sleep(K_MSEC(TASK_WDT_PERIOD_MS));
		zassert_ok(task_wdt_feed(channel_id));
	}

	wait_callback();
	zassert_equal(atomic_get(&recovery_count), 0);
	zassert_equal(task_wdt_group_action_get(&short_group), TASK_WDT_ACTION_CALLBACK);
}

ZTEST(task_wdt_group, test_leave_group)
{
	int channel_id = add_channel(&recover_group);

	/* without a group, the channel callback is called */
	zassert_ok(task_wdt_channel_group_set(channel_id, NULL));
	wait_callback();
	zassert_equal(atomic_get(&recovery_count), 0);
}

ZTEST(task_wdt_group, test_group_set_ends_recovery)
{
	int channel_id = add_channel(&recover_group);

	/* the recovery does not feed the channel, so it has to be fed within the reload period */
	atomic_set(&recovery_feed, 0);
	wait_recovery();
	k_sleep(K_MSEC(TASK_WDT_PERIOD_MS / 2U));

	/* in the new group, the channel has a full reload period again */
	zassert_ok(task_wdt_channel_group_set(channel_id, &callback_group));
	k_sleep(K_MSEC(TASK_WDT_PERIOD_MS * 3U / 4U));
	zassert_equal(atomic_get(&callback_count), 0);

	/* and a timeout calls the channel callback instead of resetting the system */
	wait_callback();
	zassert_equal(atomic_get(&recovery_count), 1);
}

ZTEST(task_wdt_group, test_group_reset_ends_recovery)
{
	int channel_id = add_channel(&recover_group);

	atomic_set(&recovery_feed, 0);
	wait_recovery();

	/* resetting the group ends the recovery, so the next timeout is recovered again */
	zassert_ok(task_wdt_group_reset(&recover_group));
	wait_recovery();
	zassert_equal(atomic_get(&callback_count), 0);
	zassert_ok(task_wdt_delete(channel_id));
}

#ifdef CONFIG_TASK_WDT_SUPERVISION
static const uint32_t next[] = {BIT(1), BIT(0)};
static const struct task_wdt_deadline deadlines[] = {{.start = 0, .end = 1}};

TASK_WDT_SUPERVISION_DEFINE(flow, next, deadlines);

ZTEST(task_wdt_group, test_recover_supervision)
{
	int channel_id = task_wdt_add_ext(TASK_WDT_WAIT_MS, fault_callback, &recovery_count);

	zassert_true(channel_id >= 0, "task_wdt_add_ext() failed: %d", channel_id);
	zassert_ok(task_wdt_channel_group_set(channel_id, &recover_group));
	zassert_ok(task_wdt_supervision_start(&flow, channel_id));

	/* checkpoint faults of the supervised entity are recovered too */
	zassert_ok(task_wdt_checkpoint(&flow, 0));
	zassert_ok(task_wdt_checkpoint(&flow, 0));
	wait_recovery();
	zassert_equal(atomic_get(&recovery_reason), TASK_WDT_FAULT_SEQUENCE);
	zassert_equal(atomic_get(&callback_count), 0);

	/* the entity starts over, so any checkpoint may be reached first */
	zassert_ok(task_wdt_checkpoint(&flow, 1));
	zassert_ok(task_wdt_checkpoint(&flow, 0));
	zassert_equal(k_sem_take(&recovery_sem, K_MSEC(TASK_WDT_PERIOD_MS)), -EAGAIN);

	zassert_ok(task_wdt_supervision_stop(&flow));
}
#endif /* CONFIG_TASK_WDT_SUPERVISION */

ZTEST(task_wdt_group, test_invalid)
{
	int channel_id = add_channel(&callback_group);

	zassert_equal(task_wdt_channel_group_set(-1, &callback_group), -EINVAL);
	zassert_equal(task_wdt_channel_group_set(CONFIG_TASK_WDT_CHANNELS, &callback_group),
		      -EINVAL);
	zassert_equal(task_wdt_channel_group_set(channel_id + 1, &callback_group), -EINVAL);
	zassert_equal(task_wdt_group_action_get(NULL), -EINVAL);
	zassert_equal(task_wdt_group_reset(NULL), -EINVAL);

	zassert_ok(task_wdt_delete(channel_id));
	zassert_equal(task_wdt_channel_group_set(channel_id, &callback_group), -EINVAL);
}

static void task_wdt_group_before(void *arg)
{
	ARG_UNUSED(arg);

	k_sem_reset(&callback_sem);
	k_sem_reset(&recovery_sem);
	atomic_set(&callback_count, 0);
	atomic_set(&recovery_count, 0);
	atomic_set(&recovery_in_isr, 0);
	atomic_set(&recovery_reason, -1);
	atomic_set(&recovery_feed, 1);
	memset(recovery_thread_name, 0, sizeof(recovery_thread_name));

	task_wdt_group_reset(&callback_group);
	task_wdt_group_reset(&recover_group);
	task_wdt_group_reset(&escalating_group);
	task_wdt_group_reset(&short_group);
}

static void task_wdt_group_after(void *arg)
{
	ARG_UNUSED(arg);

	for (int channel_id = 0; channel_id < CONFIG_TASK_WDT_CHANNELS; channel_id++) {
		task_wdt_delete(channel_id);
	}
}

#else /* CONFIG_TASK_WDT_GROUPS */

TASK_WDT_GROUP_DEFINE(callback_group, TASK_WDT_ACTION_CALLBACK, NULL, NULL);

ZTEST(task_wdt_group, test_disabled)
{
	int channel_id = task_wdt_add(TASK_WDT_WAIT_MS, NULL, NULL);

	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);
	zassert_equal(task_wdt_channel_group_set(channel_id, &callback_group), -ENOTSUP);
	zassert_equal(task_wdt_group_action_get(&callback_group), -ENOTSUP);
	zassert_equal(task_wdt_group_reset(&callback_group), -ENOTSUP);
	zassert_ok(task_wdt_delete(channel_id));
}

#define task_wdt_group_before NULL
#define task_wdt_group_after  NULL

#endif /* CONFIG_TASK_WDT_GROUPS */

static void *task_wdt_group_setup(void)
{
	zassert_ok(task_wdt_init(NULL));

	return NULL;
}

ZTEST_SUITE(task_wdt_group, NULL, task_wdt_group_setup, task_wdt_group_before, task_wdt_group_after,
	    NULL);
