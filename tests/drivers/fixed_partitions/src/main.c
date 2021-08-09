/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/flash.h>
#include <devicetree.h>
#include <storage/flash_map.h>

#define TEST_PARTITION storage
#define FLASH_TEST_REGION_OFFSET FLASH_AREA_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_SIZE DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(TEST_PARTITION))
#define TEST_AREA_MAX (FLASH_TEST_REGION_OFFSET + \
		       FLASH_AREA_SIZE(TEST_PARTITION))

static void test_all(void)
{
	int rc;

	char buffer[1024];
	char text[] = "Hello world";
	const struct device *dev = NULL;

	dev = FIXED_PARTITION_DEVICE(TEST_PARTITION);
	zassert_true(dev != NULL, "Expected pointer to device");

	zassert_true(FIXED_PARTITION_DEVICE_READY(TEST_PARTITION), "Expected device to be ready");

	rc = FIXED_PARTITION_ERASE(TEST_PARTITION, 0, TEST_PARTITION_SIZE);
	zassert_equal(0, rc, "Erase failed");

	rc = FIXED_PARTITION_ERASE(TEST_PARTITION, -1, TEST_PARTITION_SIZE);
	zassert_equal(-EINVAL, rc, "Expected fail on negative offset");

	rc = FIXED_PARTITION_ERASE(TEST_PARTITION, 0, TEST_PARTITION_SIZE + 1);
	zassert_equal(-EINVAL, rc, "Expected fail on size");

	rc = FIXED_PARTITION_ERASE(TEST_PARTITION, -1, TEST_PARTITION_SIZE + 1);
	zassert_equal(-EINVAL, rc, "Expected fail");

	rc = FIXED_PARTITION_WRITE(TEST_PARTITION, 0, text, sizeof(text));
	zassert_equal(0, rc, "Write failed");

	rc = FIXED_PARTITION_WRITE(TEST_PARTITION, -1, text, sizeof(text));
	zassert_equal(-EINVAL, rc, "Expected fail on negative offset");

	rc = FIXED_PARTITION_WRITE(TEST_PARTITION, TEST_PARTITION_SIZE - sizeof(text), text,
				   sizeof(text) + 1);
	zassert_equal(-EINVAL, rc, "Expected fail on size");

	rc = FIXED_PARTITION_WRITE(TEST_PARTITION, -1, text, sizeof(text) + 1);
	zassert_equal(-EINVAL, rc, "Expected fail");

	rc = FIXED_PARTITION_READ(TEST_PARTITION, 0, buffer, sizeof(text));
	zassert_equal(0, rc, "Read failed");

	rc = FIXED_PARTITION_READ(TEST_PARTITION, -1, buffer, sizeof(text));
	zassert_equal(-EINVAL, rc, "Expected fail on negative offset");
	zassert_equal(0, memcmp(text, buffer, sizeof(text)), "Read value bad");

	rc = FIXED_PARTITION_READ(TEST_PARTITION, 0, buffer, TEST_PARTITION_SIZE + 1);
	zassert_equal(-EINVAL, rc, "Expected fail reading past the partition size");
}


void test_main(void)
{
	ztest_test_suite(fixed_partition_macro_test,
		ztest_unit_test(test_all)
	);

	ztest_run_test_suite(fixed_partition_macro_test);
}
