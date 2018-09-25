/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <device.h>
#include <power.h>

#define SLEEP_MS 1000

static enum
{
	PM_BUSY_CHECK = 0,
	PM_ANY_BUSY_CHECK,
	PM_IDLE
} pstate;
static struct device *device_list;
static int device_count;

static void tdevice_busycheck(int idx, bool busy)
{
	if (idx == -1) {
		zassert_equal(device_any_busy_check(), busy, NULL);
	} else {
		zassert_equal(device_busy_check(&device_list[idx]), busy, NULL);
	}
}

int _sys_soc_suspend(s32_t ticks)
{
	/* PMA NOTE: busy check supposed to be invoked from _sys_soc_suspend()
	 * only, for the reason to have "busy check and act" not being
	 * interfered by "busy set" from any drivers
	 */
	switch (pstate) {
	case PM_BUSY_CHECK:
		tdevice_busycheck(0, true);
		pstate = PM_IDLE;
		return SYS_PM_NOT_HANDLED;
	case PM_ANY_BUSY_CHECK:
		tdevice_busycheck(-1, true);
		pstate = PM_IDLE;
		return SYS_PM_NOT_HANDLED;
	default:
		return SYS_PM_NOT_HANDLED;
	}
}

void _sys_soc_resume(void)
{
}

/* test cases*/
void test_device_list_get(void)
{
	/**TESTPOINT: device list get*/
	device_list_get(&device_list, &device_count);
	/*at least one device in the list: "system clock"*/
	zassert_true(device_count > 0, NULL);
	for (int i = 0; i < device_count; i++) {
		TC_PRINT("%s\n", device_list[i].config->name);
	}

}

void test_device_busy_set(void)
{
	/**TESTPOINT: set device busy*/
	device_busy_set(&device_list[0]);
}

void test_device_busy_check(void)
{
	pstate = PM_BUSY_CHECK;

	/**TESTPOINT: check device busy*/
	k_sleep(SLEEP_MS);
}

void test_device_any_busy_check(void)
{
	pstate = PM_ANY_BUSY_CHECK;

	/**TESTPOING check any device busy*/
	k_sleep(SLEEP_MS);
}

void test_device_busy_clear(void)
{
	/**TESTPOINT: clear device busy*/
	device_busy_clear(&device_list[0]);
	tdevice_busycheck(0, false);
	tdevice_busycheck(-1, false);
}
