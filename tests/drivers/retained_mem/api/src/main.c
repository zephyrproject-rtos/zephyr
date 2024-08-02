/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/ztest.h>
#include <errno.h>

const static struct device *retained_mem_test_device =	DEVICE_DT_GET(DT_ALIAS(
									retainedmemtestdevice));

#if CONFIG_RETAINED_MEM_SIZE_LIMITED
/* For size-limited tests, use data size of 1 byte */
static uint8_t data[1] = {
	0x5b,
};
static uint8_t empty_data[1] = {
	0x00,
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
static uint8_t buffer[10];
#endif

ZTEST(retained_mem_api, test_read_write)
{
	int32_t rc;

	rc = retained_mem_write(retained_mem_test_device, 0, data, sizeof(data));
	zassert_equal(rc, 0, "Return code should be success");

	memset(buffer, 0, sizeof(buffer));

	rc = retained_mem_read(retained_mem_test_device, 0, buffer, sizeof(buffer));
	zassert_equal(rc, 0, "Return code should be success");

	zassert_mem_equal(data, buffer, sizeof(data), "Expected written data to match");
}

ZTEST(retained_mem_api, test_size)
{
	int32_t rc;

	rc = retained_mem_size(retained_mem_test_device);
	zassume_between_inclusive(rc, 1, 0x4000, "Retained memory size is not valid");
}

ZTEST(retained_mem_api, test_clear)
{
	int32_t rc;

	rc = retained_mem_clear(retained_mem_test_device);
	zassert_equal(rc, 0, "Return code should be success");

	rc = retained_mem_write(retained_mem_test_device, 0, data, sizeof(data));
	zassert_equal(rc, 0, "Return code should be success");

	rc = retained_mem_read(retained_mem_test_device, 0, buffer, sizeof(buffer));
	zassert_equal(rc, 0, "Return code should be success");

	zassert_mem_equal(data, buffer, sizeof(data), "Expected written data to match");

	rc = retained_mem_clear(retained_mem_test_device);
	zassert_equal(rc, 0, "Return code should be success");

	rc = retained_mem_read(retained_mem_test_device, 0, buffer, sizeof(buffer));
	zassert_equal(rc, 0, "Return code should be success");

	zassert_mem_equal(empty_data, buffer, sizeof(empty_data), "Expected data to be 0x00's");
}

ZTEST_SUITE(retained_mem_api, NULL, NULL, NULL, NULL, NULL);
