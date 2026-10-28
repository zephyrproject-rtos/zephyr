/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/sys/reboot.h>

const static struct device *retained_memory_device = DEVICE_DT_GET(DT_PARENT(DT_CHOSEN(
									zephyr_boot_mode)));

#define TEST_STATUS_NORMAL 0x0
#define TEST_STATUS_REBOOT1 0x184be001
#define TEST_STATUS_REBOOT2 0x391b
#define BOOTMODE_NORMAL 0x0
#define BOOTMODE_VALUE1 0x14
#define BOOTMODE_VALUE2 0xf1
volatile uint32_t test_status __attribute__((section(".noinit.test")));

ZTEST(retention_bootmode_reboot, test_bootmode)
{
	int32_t rc;

	if (test_status == TEST_STATUS_REBOOT1) {
		test_status = BOOTMODE_VALUE2;

		rc = bootmode_check(BOOTMODE_VALUE1);
		zassert_equal(rc, 1, "Boot mode should be set to test value 1: %d", rc);

		rc = bootmode_check(BOOTMODE_VALUE2);
		zassert_equal(rc, 0, "Boot mode should not be set to test value 2: %d", rc);

		rc = bootmode_check(BOOTMODE_NORMAL);
		zassert_equal(rc, 0, "Boot mode should not be set to default value: %d", rc);

		rc = bootmode_set(BOOTMODE_VALUE2);
		zassert_equal(rc, 0, "Boot mode set should work: %d", rc);

		test_status = TEST_STATUS_REBOOT2;

		sys_reboot(SYS_REBOOT_COLD);
	} else if (test_status == TEST_STATUS_REBOOT2) {
		test_status = BOOTMODE_NORMAL;

		rc = bootmode_check(BOOTMODE_VALUE1);
		zassert_equal(rc, 0, "Boot mode should not be set to test value 1: %d", rc);

		rc = bootmode_check(BOOTMODE_VALUE2);
		zassert_equal(rc, 1, "Boot mode should be set to test value 2: %d", rc);

		rc = bootmode_check(BOOTMODE_NORMAL);
		zassert_equal(rc, 0, "Boot mode should not be set to default value: %d", rc);

		/*
		 * Use retained memory directly to ensure checksum and prefix is cleared after test
		 */
		rc = retained_mem_clear(retained_memory_device);
		zassert_equal(rc, 0, "Return code should be success: %d", rc);
	} else {
		rc = bootmode_check(BOOTMODE_VALUE1);
		zassert_equal(rc, 0, "Boot mode should not be set to test value 1: %d", rc);

		rc = bootmode_check(BOOTMODE_VALUE2);
		zassert_equal(rc, 0, "Boot mode should not be set to test value 2: %d", rc);

		rc = bootmode_set(BOOTMODE_VALUE1);
		zassert_equal(rc, 0, "Boot mode set should work: %d", rc);

		test_status = TEST_STATUS_REBOOT1;

		sys_reboot(SYS_REBOOT_COLD);
	}
}

ZTEST_SUITE(retention_bootmode_reboot, NULL, NULL, NULL, NULL, NULL);
