/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * pm_flash_check: RW612 low-power mode x flash health sample.
 *
 * Enters each Zephyr low-power state (PM1 idle, PM2 suspend-to-idle,
 * PM3 standby) in turn and actively erases/writes/reads back the external
 * NOR flash before and after every entry/exit, printing a per-mode PASS/FAIL
 * and a final summary.
 *
 * The judgment is purely functional: after waking from a low-power mode the
 * flash API (erase/write/read) must keep working.  PM3 wake resets the
 * FlexSPI controller configuration, so this exercises the flash driver's
 * power-domain restore path.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/storage/flash_map.h>

/* Scratch area: the "scratch" fixed partition (48 MiB into the 64 MiB NOR). */
#define SCRATCH_OFFSET      PARTITION_OFFSET(scratch_partition)
#define SCRATCH_SECTOR_SIZE 0x1000u
#define SCRATCH_LEN         256u

static const struct device *flash_dev;
static int failures;
static enum pm_state test_state;

static void pm_notify_entry(enum pm_state state)
{
	if (state == test_state) {
		printk("[pm] state_entry=%d\n", state);
	}
}

static void pm_notify_exit(enum pm_state state)
{
	if (state == test_state) {
		printk("[pm] state_exit=%d\n", state);
	}
}

static struct pm_notifier notifier = {
	.state_entry = pm_notify_entry,
	.state_exit = pm_notify_exit,
};

/*
 * Erase, write and read back a scratch sector.  Returns 0 when the flash
 * behaves correctly, -1 otherwise.
 */
static int flash_health_check(const char *tag)
{
	static uint8_t wbuf[SCRATCH_LEN] __aligned(4);
	static uint8_t rbuf[SCRATCH_LEN] __aligned(4);
	int ret;
	uint32_t i;

	for (i = 0; i < SCRATCH_LEN; i++) {
		wbuf[i] = (uint8_t)(i * 7u + 1u);
	}

	ret = flash_erase(flash_dev, SCRATCH_OFFSET, SCRATCH_SECTOR_SIZE);
	if (ret != 0) {
		printk("[flash][%s] erase FAILED ret=%d\n", tag, ret);
		return -1;
	}
	ret = flash_write(flash_dev, SCRATCH_OFFSET, wbuf, SCRATCH_LEN);
	if (ret != 0) {
		printk("[flash][%s] write FAILED ret=%d\n", tag, ret);
		return -1;
	}
	ret = flash_read(flash_dev, SCRATCH_OFFSET, rbuf, SCRATCH_LEN);
	if (ret != 0) {
		printk("[flash][%s] read FAILED ret=%d\n", tag, ret);
		return -1;
	}
	if (memcmp(wbuf, rbuf, SCRATCH_LEN) != 0) {
		printk("[flash][%s] readback MISMATCH\n", tag);
		return -1;
	}
	printk("[flash][%s] erase/write/readback OK\n", tag);
	return 0;
}

static void run_low_power_test(const char *name, enum pm_state state, k_timeout_t sleep)
{
	const struct pm_state_info info = {
		.state = state,
		.substate_id = 0,
	};

	printk("=== %s (pm_state=%d) ===\n", name, state);
	test_state = state;
	if (flash_health_check("before") != 0) {
		failures++;
		return;
	}

	if (!pm_state_force(0, &info)) {
		printk("[%s] pm_state_force failed\n", name);
		failures++;
		return;
	}
	printk("[%s] entering low-power state...\n", name);
	k_sleep(sleep);
	printk("[%s] woke up\n", name);

	if (flash_health_check("after") != 0) {
		failures++;
		return;
	}

	printk("[%s] PASS\n", name);
}

int main(void)
{
	flash_dev = DEVICE_DT_GET(DT_NODELABEL(ext_flash_ctrl));
	if (!device_is_ready(flash_dev)) {
		printk("[pm_flash_check] flash device not ready\n");
		return 0;
	}

	pm_notifier_register(&notifier);

	run_low_power_test("PM1 RUNTIME_IDLE", PM_STATE_RUNTIME_IDLE, K_MSEC(100));
	run_low_power_test("PM2 SUSPEND_TO_IDLE", PM_STATE_SUSPEND_TO_IDLE, K_MSEC(200));
	run_low_power_test("PM3 STANDBY", PM_STATE_STANDBY, K_MSEC(2000));

	/*
	 * The last pm_state_force() persists, so returning to the idle thread
	 * would re-enter a low-power state.  Lock out STANDBY (which would
	 * release the SWD debug port) and SUSPEND_TO_IDLE so the idle thread
	 * stays in the lightest state.  Print the summary only after the locks
	 * are held so the final line is not lost to a low-power entry (which
	 * would confuse the CI harness), then sleep forever.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	printk("=== SUMMARY: %s (%d failures) ===\n", failures == 0 ? "PASS" : "FAIL", failures);
	k_sleep(K_FOREVER);
}
