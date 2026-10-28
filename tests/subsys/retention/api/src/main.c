/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/retention/retention.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/sys/reboot.h>

const static struct device *retention_test_device = DEVICE_DT_GET(DT_ALIAS(retentiontestdevice));
const static struct device *retained_memory_device = DEVICE_DT_GET(DT_PARENT(DT_ALIAS(
									retentiontestdevice)));

#define TEST_STATUS_NORMAL 0x0
#define TEST_STATUS_REBOOT 0x2f1ba0
volatile uint32_t test_status __attribute__((section(".noinit.test")));

#if CONFIG_RETENTION_SIZE_LIMITED
/* For size-limited tests, use data size of 1 byte */
static uint8_t data[1] = {
	0x5b,
};
static uint8_t empty_data[1] = {
	0x00,
};
static uint8_t reboot_data[1] = {
	0xe8,
};
static uint8_t buffer[1];
#else
/* For other devices, use data size of 10 bytes */
static uint8_t data[10] = {
	0x23, 0x82, 0xa8, 0x7b, 0xde, 0x18, 0x00, 0xff, 0x8e, 0xd6,
};
static uint8_t empty_data[10] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static uint8_t reboot_data[10] = {
	0xf1, 0xb2, 0x13, 0x00, 0xff, 0x22, 0x3f, 0x1b, 0x58, 0xab,
};
static uint8_t buffer[10];
#endif

ZTEST(retention_api, test_read_write)
{
	int32_t rc;

	rc = retention_write(retention_test_device, 0, data, sizeof(data));
	zassert_equal(rc, 0, "Return code should be success: %d", rc);

	memset(buffer, 0, sizeof(buffer));

	rc = retention_read(retention_test_device, 0, buffer, sizeof(buffer));
	zassert_equal(rc, 0, "Return code should be success: %d", rc);

	zassert_mem_equal(data, buffer, sizeof(data), "Expected written data to match");
}

ZTEST(retention_api, test_size)
{
	int32_t rc;

	rc = retention_size(retention_test_device);
	zassume_between_inclusive(rc, 1, 0x4000, "Retention size is not valid: %d", rc);
}

ZTEST(retention_api, test_clear)
{
	int32_t rc;

	rc = retention_clear(retention_test_device);
	zassert_equal(rc, 0, "Return code should be success: %d", rc);

	rc = retention_write(retention_test_device, 0, data, sizeof(data));
	zassert_equal(rc, 0, "Return code should be success: %d", rc);

	rc = retention_read(retention_test_device, 0, buffer, sizeof(buffer));
	zassert_equal(rc, 0, "Return code should be success: %d", rc);

	zassert_mem_equal(data, buffer, sizeof(data), "Expected written data to match");

	rc = retention_clear(retention_test_device);
	zassert_equal(rc, 0, "Return code should be success: %d", rc);

	rc = retention_read(retention_test_device, 0, buffer, sizeof(buffer));
	zassert_equal(rc, 0, "Return code should be success: %d", rc);

	zassert_mem_equal(empty_data, buffer, sizeof(empty_data), "Expected data to be 0x00's");
}

ZTEST(a_retention_api_reboot, test_reboot)
{
	int32_t rc;

	if (test_status == TEST_STATUS_REBOOT) {
		test_status = TEST_STATUS_NORMAL;

		rc = retention_is_valid(retention_test_device);
		zassert_equal(rc, 1, "Retention data should be valid: %d", rc);

		rc = retention_read(retention_test_device, 0, buffer, sizeof(buffer));
		zassert_equal(rc, 0, "Return code should be success: %d", rc);

		/*
		 * Use retained memory directly to ensure checksum and prefix is clear also, other
		 * parts of the test check retention_clear()
		 */
		rc = retained_mem_clear(retained_memory_device);
		zassert_equal(rc, 0, "Return code should be success: %d", rc);

		zassert_mem_equal(reboot_data, buffer, sizeof(reboot_data),
				  "Expected written data to match over reboot");
	} else {
		rc = retention_is_valid(retention_test_device);
		zassert_equal(rc, 0, "Retention data should not be valid: %d", rc);

		rc = retention_clear(retention_test_device);
		zassert_equal(rc, 0, "Return code should be success: %d", rc);

		rc = retention_write(retention_test_device, 0, reboot_data, sizeof(reboot_data));
		zassert_equal(rc, 0, "Return code should be success: %d", rc);

		test_status = TEST_STATUS_REBOOT;

		sys_reboot(SYS_REBOOT_COLD);
	}
}

/* Use 2 tests so that the reboot test does not get interrupted with another test after reboot */
ZTEST_SUITE(a_retention_api_reboot, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(retention_api, NULL, NULL, NULL, NULL, NULL);
