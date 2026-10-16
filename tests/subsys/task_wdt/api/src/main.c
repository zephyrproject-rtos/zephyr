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

#ifdef CONFIG_TASK_WDT_THREAD_INFO
#include <zephyr/logging/log_backend.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#endif

#define TASK_WDT_PERIOD_MS 100U
#define TASK_WDT_WAIT_MS   500U

K_SEM_DEFINE(callback_sem, 0, 1);
static atomic_t callback_count;
static atomic_t callback_channel;
static atomic_ptr_t callback_user_data;

static void task_wdt_callback(int channel_id, void *user_data)
{
	atomic_inc(&callback_count);
	atomic_set(&callback_channel, channel_id);
	atomic_ptr_set(&callback_user_data, user_data);
	k_sem_give(&callback_sem);
}

static void *task_wdt_setup(void)
{
	int ret = task_wdt_init(NULL);

	zassert_equal(ret, 0, "task_wdt_init() failed: %d", ret);

	return NULL;
}

static void task_wdt_before(void *arg)
{
	ARG_UNUSED(arg);

	k_sem_reset(&callback_sem);
	atomic_set(&callback_count, 0);
	atomic_set(&callback_channel, -1);
	atomic_ptr_set(&callback_user_data, NULL);
}

static void task_wdt_after(void *arg)
{
	ARG_UNUSED(arg);

	for (int channel_id = 0; channel_id < CONFIG_TASK_WDT_CHANNELS; channel_id++) {
		task_wdt_delete(channel_id);
	}
}

ZTEST(task_wdt, test_timeout)
{
	static int user_data;
	int channel_id;
	int ret;

	channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, task_wdt_callback, &user_data);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	ret = k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(ret, 0, "watchdog callback did not run");
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(atomic_get(&callback_channel), channel_id);
	zassert_equal(atomic_ptr_get(&callback_user_data), &user_data);
}

ZTEST(task_wdt, test_feed)
{
	int channel_id;
	int ret;

	channel_id = task_wdt_add(200U, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	k_sleep(K_MSEC(50));
	ret = task_wdt_feed(channel_id);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(100));
	zassert_equal(atomic_get(&callback_count), 0);

	ret = k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(ret, 0, "watchdog callback did not run after feed");
	zassert_equal(atomic_get(&callback_count), 1);
}

ZTEST(task_wdt, test_delete)
{
	int channel_id;
	int ret;

	channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	ret = task_wdt_delete(channel_id);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(atomic_get(&callback_count), 0);
}

ZTEST(task_wdt, test_channel_info)
{
	struct task_wdt_channel_info info;
	int channel_id;
	int ret;

	ret = task_wdt_channel_info_get(0, NULL);
	zassert_equal(ret, -EINVAL);
	ret = task_wdt_channel_info_get(CONFIG_TASK_WDT_CHANNELS, &info);
	zassert_equal(ret, -EINVAL);
	ret = task_wdt_channel_info_get(0, &info);
	zassert_equal(ret, -EINVAL, "unused channel reported");

	channel_id = task_wdt_add(1000U, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	ret = task_wdt_channel_info_get(channel_id, &info);
	zassert_equal(ret, 0);
	zassert_equal(info.reload_period, 1000U);
	zassert_true(info.time_left > 500U && info.time_left <= 1000U, "unexpected time left: %u",
		     info.time_left);
#ifdef CONFIG_TASK_WDT_THREAD_INFO
	zassert_equal(info.thread, k_current_get());
#else
	zassert_is_null(info.thread);
	zassert_equal(task_wdt_channel_thread_set(channel_id, k_current_get()), -ENOTSUP);
#endif
}

ZTEST(task_wdt, test_suspend_resume)
{
	int channel_id;
	int ret;

	channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	task_wdt_suspend();
	k_sleep(K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(atomic_get(&callback_count), 0);

	task_wdt_resume();
	ret = k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(ret, 0, "watchdog callback did not run after resume");
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(atomic_get(&callback_channel), channel_id);
}

#ifdef CONFIG_TASK_WDT_FAULT_REASONS

static struct task_wdt_fault last_fault;

static void task_wdt_fault_callback(const struct task_wdt_fault *fault, void *user_data)
{
	last_fault = *fault;
	task_wdt_callback(fault->channel_id, user_data);
}

ZTEST(task_wdt, test_fault_timeout)
{
	static int user_data;
	int channel_id;
	int ret;

	channel_id = task_wdt_add_ext(TASK_WDT_PERIOD_MS, task_wdt_fault_callback, &user_data);
	zassert_true(channel_id >= 0, "task_wdt_add_ext() failed: %d", channel_id);

	ret = k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(ret, 0, "watchdog callback did not run");
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(atomic_ptr_get(&callback_user_data), &user_data);
	zassert_equal(last_fault.channel_id, channel_id);
	zassert_equal(last_fault.reason, TASK_WDT_FAULT_TIMEOUT);
#ifdef CONFIG_TASK_WDT_THREAD_INFO
	zassert_equal(last_fault.thread, k_current_get());
#else
	zassert_is_null(last_fault.thread);
#endif
}

ZTEST(task_wdt, test_window_set)
{
	int channel_id;

	zassert_equal(task_wdt_window_set(-1, 10U), -EINVAL);
	zassert_equal(task_wdt_window_set(0, 10U), -EINVAL, "unused channel accepted");

	channel_id = task_wdt_add_ext(TASK_WDT_PERIOD_MS, task_wdt_fault_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add_ext() failed: %d", channel_id);

	zassert_equal(task_wdt_window_set(channel_id, TASK_WDT_PERIOD_MS), -EINVAL);
	zassert_ok(task_wdt_window_set(channel_id, TASK_WDT_PERIOD_MS / 2U));
	zassert_ok(task_wdt_window_set(channel_id, 0U));
}

ZTEST(task_wdt, test_feed_deleted)
{
	int channel_id;

	channel_id = task_wdt_add_ext(1000U, task_wdt_fault_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add_ext() failed: %d", channel_id);
	zassert_ok(task_wdt_window_set(channel_id, 100U));
	zassert_ok(task_wdt_delete(channel_id));

	/* a deleted channel cannot be fed, so it does not report a fault either */
	zassert_equal(task_wdt_feed(channel_id), -EINVAL);
	zassert_equal(task_wdt_feed(channel_id), -EINVAL);
	zassert_equal(atomic_get(&callback_count), 0);
}

ZTEST(task_wdt, test_fault_too_early)
{
	int channel_id;

	channel_id = task_wdt_add_ext(1000U, task_wdt_fault_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add_ext() failed: %d", channel_id);
	zassert_ok(task_wdt_window_set(channel_id, 100U));

	/* feeding after the minimum interval is fine */
	k_sleep(K_MSEC(150));
	zassert_ok(task_wdt_feed(channel_id));
	zassert_equal(atomic_get(&callback_count), 0);

	/* feeding again right away is a fault, reported before task_wdt_feed() returns */
	zassert_ok(task_wdt_feed(channel_id));
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(last_fault.channel_id, channel_id);
	zassert_equal(last_fault.reason, TASK_WDT_FAULT_TOO_EARLY);

	/* the channel was fed anyway */
	k_sleep(K_MSEC(150));
	zassert_ok(task_wdt_feed(channel_id));
	zassert_equal(atomic_get(&callback_count), 1);
}

ZTEST(task_wdt, test_too_early_legacy_callback)
{
	int channel_id;

	channel_id = task_wdt_add(1000U, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);
	zassert_ok(task_wdt_window_set(channel_id, 100U));

	zassert_ok(task_wdt_feed(channel_id));
	zassert_equal(atomic_get(&callback_count), 1);
	zassert_equal(atomic_get(&callback_channel), channel_id);
}

#else

ZTEST(task_wdt, test_fault_reasons_disabled)
{
	zassert_equal(task_wdt_add_ext(TASK_WDT_PERIOD_MS, NULL, NULL), -ENOTSUP);
	zassert_equal(task_wdt_window_set(0, 10U), -ENOTSUP);
}

#endif /* CONFIG_TASK_WDT_FAULT_REASONS */

#ifdef CONFIG_TASK_WDT_THREAD_INFO

#define FEEDER_STACK_SIZE 1024
#define FEEDER_PRIORITY   K_PRIO_PREEMPT(0)

K_THREAD_STACK_DEFINE(feeder_stack, FEEDER_STACK_SIZE);
static struct k_thread feeder_thread;

static void feeder_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	task_wdt_feed(POINTER_TO_INT(p1));
}

/* Feed the channel once from a separate thread named "feeder" */
static k_tid_t feed_from_feeder(int channel_id)
{
	k_tid_t tid;

	tid = k_thread_create(&feeder_thread, feeder_stack, K_THREAD_STACK_SIZEOF(feeder_stack),
			      feeder_entry, INT_TO_POINTER(channel_id), NULL, NULL, FEEDER_PRIORITY,
			      0, K_FOREVER);
	k_thread_name_set(tid, "feeder");
	k_thread_start(tid);
	zassert_ok(k_thread_join(tid, K_FOREVER));

	return tid;
}

ZTEST(task_wdt, test_owner_last_feeder)
{
	struct task_wdt_channel_info info;
	k_tid_t feeder;
	int channel_id;

	channel_id = task_wdt_add(1000U, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	zassert_ok(task_wdt_channel_info_get(channel_id, &info));
	zassert_equal(info.thread, k_current_get());

	feeder = feed_from_feeder(channel_id);
	zassert_ok(task_wdt_channel_info_get(channel_id, &info));
	zassert_equal(info.thread, feeder);

	zassert_ok(task_wdt_feed(channel_id));
	zassert_ok(task_wdt_channel_info_get(channel_id, &info));
	zassert_equal(info.thread, k_current_get());
}

ZTEST(task_wdt, test_owner_set)
{
	struct task_wdt_channel_info info;
	int channel_id;

	zassert_equal(task_wdt_channel_thread_set(-1, NULL), -EINVAL);
	zassert_equal(task_wdt_channel_thread_set(0, NULL), -EINVAL, "unused channel accepted");

	channel_id = task_wdt_add(1000U, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	/* An explicit owner is kept when another thread feeds the channel */
	zassert_ok(task_wdt_channel_thread_set(channel_id, k_current_get()));
	feed_from_feeder(channel_id);
	zassert_ok(task_wdt_channel_info_get(channel_id, &info));
	zassert_equal(info.thread, k_current_get());

	/* Clearing the owner goes back to the last feeder */
	zassert_ok(task_wdt_channel_thread_set(channel_id, NULL));
	zassert_ok(task_wdt_channel_info_get(channel_id, &info));
	zassert_is_null(info.thread);
	feed_from_feeder(channel_id);
	zassert_ok(task_wdt_channel_info_get(channel_id, &info));
	zassert_equal(info.thread, &feeder_thread);
}

/* Log backend keeping the task_wdt messages for inspection */
static char log_text[512];
static size_t log_len;

static int log_text_out(int c, void *ctx)
{
	ARG_UNUSED(ctx);

	if (log_len < sizeof(log_text) - 1) {
		log_text[log_len++] = (char)c;
		log_text[log_len] = '\0';
	}

	return c;
}

static void capture_process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	size_t len;
	uint8_t *package = log_msg_get_package(&msg->log, &len);

	ARG_UNUSED(backend);

	cbpprintf(log_text_out, NULL, package);
	log_text_out('\n', NULL);
}

static void capture_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api capture_api = {
	.process = capture_process,
	.panic = capture_panic,
};

LOG_BACKEND_DEFINE(capture_backend, capture_api, true);

ZTEST(task_wdt, test_timeout_report)
{
	int channel_id;
	int ret;

	channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);
	feed_from_feeder(channel_id);

	log_len = 0;
	log_text[0] = '\0';

	ret = k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS));
	zassert_equal(ret, 0, "watchdog callback did not run");

	zassert_not_null(strstr(log_text, "timed out"), "no timeout report: %s", log_text);
	zassert_not_null(strstr(log_text, "(feeder)"), "owner not reported: %s", log_text);
	zassert_not_null(strstr(log_text, "bytes of stack unused"), "stack usage not reported: %s",
			 log_text);
}

ZTEST(task_wdt, test_shell_list)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *output;
	size_t size;
	int channel_id;

	channel_id = task_wdt_add(1000U, task_wdt_callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);
	feed_from_feeder(channel_id);

	shell_backend_dummy_clear_output(sh);
	zassert_ok(shell_execute_cmd(sh, "task_wdt list"));
	output = shell_backend_dummy_get_output(sh, &size);

	zassert_not_null(strstr(output, "1000"), "period not listed: %s", output);
	zassert_not_null(strstr(output, "feeder"), "owner not listed: %s", output);
}

#endif /* CONFIG_TASK_WDT_THREAD_INFO */

ZTEST_SUITE(task_wdt, NULL, task_wdt_setup, task_wdt_before, task_wdt_after, NULL);
