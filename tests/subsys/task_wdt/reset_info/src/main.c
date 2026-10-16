/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/ztest.h>

#define BOOT_MAGIC         0x52425354U
#define TASK_WDT_PERIOD_MS 100U
#define TASK_WDT_WAIT_MS   1000U

/* Each boot runs the next stage of the test, the system is reset in between */
enum stage {
	STAGE_TIMEOUT,
	STAGE_REPEATED_TIMEOUT,
	STAGE_CALLBACK,
	STAGE_SOFTWARE_RESET,
	STAGE_CALLBACK_RESET,
	STAGE_TOO_EARLY,
	STAGE_SEQUENCE,
	STAGE_DONE,
};

static struct {
	uint32_t magic;
	uint32_t stage;
} boot __noinit;

static K_SEM_DEFINE(callback_sem, 0, 1);

static void next_stage(enum stage stage)
{
	boot.magic = BOOT_MAGIC;
	boot.stage = stage;
}

static void returning_callback(int channel_id, void *user_data)
{
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);

	k_sem_give(&callback_sem);
}

static void resetting_callback(int channel_id, void *user_data)
{
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);

	sys_reboot(SYS_REBOOT_COLD);
}

/* Let a channel owned by a thread named "victim" time out */
static void time_out(task_wdt_callback_t callback)
{
	int channel_id;

	k_thread_name_set(k_current_get(), "victim");

	channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, callback, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);

	k_sleep(K_MSEC(TASK_WDT_WAIT_MS));
	zassert_unreachable("no reset after the channel timed out");
}

/* Feed a channel owned by a thread named "victim" too early */
static void feed_too_early(void)
{
	int channel_id;

	k_thread_name_set(k_current_get(), "victim");

	channel_id = task_wdt_add(TASK_WDT_WAIT_MS, NULL, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);
	zassert_ok(task_wdt_window_set(channel_id, TASK_WDT_PERIOD_MS));

	task_wdt_feed(channel_id);
	zassert_unreachable("no reset after the channel was fed too early");
}

static const uint32_t next[] = {BIT(1), BIT(0)};
/* a deadline without time checks */
static const struct task_wdt_deadline deadlines[] = {{.start = 0, .end = 1}};

TASK_WDT_SUPERVISION_DEFINE(flow, next, deadlines);

/* Reach a checkpoint out of sequence on a channel owned by a thread named "victim" */
static void break_sequence(void)
{
	int channel_id;

	k_thread_name_set(k_current_get(), "victim");

	channel_id = task_wdt_add(TASK_WDT_WAIT_MS, NULL, NULL);
	zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);
	zassert_ok(task_wdt_supervision_start(&flow, channel_id));
	zassert_ok(task_wdt_feed(channel_id));

	zassert_ok(task_wdt_checkpoint(&flow, 0));
	task_wdt_checkpoint(&flow, 0);
	zassert_unreachable("no reset after a checkpoint out of sequence");
}

static void check_info(enum task_wdt_reset_reason reason, uint32_t fault_count,
		       uint64_t min_uptime_ms)
{
	struct task_wdt_reset_info info;

	zassert_ok(task_wdt_reset_info_get(&info));
	zassert_equal(info.reason, reason);
	zassert_equal(info.channel_id, 0);
	zassert_equal(info.fault_count, fault_count);
	zassert_true(info.uptime_ms >= min_uptime_ms, "uptime %llu ms",
		     (unsigned long long)info.uptime_ms);
	zassert_str_equal(info.thread_name, "victim");
}

static void check_timeout_info(uint32_t fault_count)
{
	check_info(TASK_WDT_RESET_TIMEOUT, fault_count, TASK_WDT_PERIOD_MS);
}

ZTEST(task_wdt_reset_info, test_reset_info)
{
	struct task_wdt_reset_info info;
	int channel_id;

	zassert_equal(task_wdt_reset_info_get(NULL), -EINVAL);

	if (boot.magic != BOOT_MAGIC) {
		next_stage(STAGE_TIMEOUT);
	}

	TC_PRINT("Stage %u\n", boot.stage);

	switch (boot.stage) {
	case STAGE_TIMEOUT:
		zassert_ok(task_wdt_reset_info_clear());
		zassert_equal(task_wdt_reset_info_get(&info), -ENOENT);

		next_stage(STAGE_REPEATED_TIMEOUT);
		time_out(NULL);
		break;

	case STAGE_REPEATED_TIMEOUT:
		check_timeout_info(1U);

		next_stage(STAGE_CALLBACK);
		time_out(NULL);
		break;

	case STAGE_CALLBACK:
		check_timeout_info(2U);

		/* a callback handling the timeout does not leave a record */
		channel_id = task_wdt_add(TASK_WDT_PERIOD_MS, returning_callback, NULL);
		zassert_true(channel_id >= 0, "task_wdt_add() failed: %d", channel_id);
		zassert_ok(k_sem_take(&callback_sem, K_MSEC(TASK_WDT_WAIT_MS)));
		zassert_ok(task_wdt_delete(channel_id));

		next_stage(STAGE_SOFTWARE_RESET);
		sys_reboot(SYS_REBOOT_COLD);
		break;

	case STAGE_SOFTWARE_RESET:
		/* other resets are not reported and restart the fault count */
		zassert_equal(task_wdt_reset_info_get(&info), -ENOENT);

		next_stage(STAGE_CALLBACK_RESET);
		time_out(resetting_callback);
		break;

	case STAGE_CALLBACK_RESET:
		/* a callback resetting the system leaves a record */
		check_timeout_info(1U);

		zassert_ok(task_wdt_reset_info_clear());
		zassert_equal(task_wdt_reset_info_get(&info), -ENOENT);

		next_stage(STAGE_TOO_EARLY);
		feed_too_early();
		break;

	case STAGE_TOO_EARLY:
		/* feeding too early is reported as its own reset reason */
		check_info(TASK_WDT_RESET_TOO_EARLY, 1U, 0U);

		zassert_ok(task_wdt_reset_info_clear());

		next_stage(STAGE_SEQUENCE);
		break_sequence();
		break;

	case STAGE_SEQUENCE:
		/* checkpoint faults are reported as their own reset reason */
		check_info(TASK_WDT_RESET_SEQUENCE, 1U, 0U);

		zassert_ok(task_wdt_reset_info_clear());

		next_stage(STAGE_DONE);
		break;

	default:
		zassert_unreachable("unexpected stage %u", boot.stage);
	}
}

static void *task_wdt_reset_info_setup(void)
{
	zassert_ok(task_wdt_init(NULL));

	return NULL;
}

ZTEST_SUITE(task_wdt_reset_info, NULL, task_wdt_reset_info_setup, NULL, NULL, NULL);
