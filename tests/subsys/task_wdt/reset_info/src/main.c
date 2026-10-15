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

static void check_timeout_info(uint32_t fault_count)
{
	struct task_wdt_reset_info info;

	zassert_ok(task_wdt_reset_info_get(&info));
	zassert_equal(info.reason, TASK_WDT_RESET_TIMEOUT);
	zassert_equal(info.channel_id, 0);
	zassert_equal(info.fault_count, fault_count);
	zassert_true(info.uptime_ms >= TASK_WDT_PERIOD_MS, "uptime %llu ms",
		     (unsigned long long)info.uptime_ms);
	zassert_str_equal(info.thread_name, "victim");
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
