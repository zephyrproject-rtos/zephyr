/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#define MAGIC_STRING "flash_data"

#define TEST_AREA_OFFSET	PARTITION_OFFSET(storage_partition)
#define TEST_AREA_SIZE		PARTITION_SIZE(storage_partition)
#define TEST_AREA_MAX		(TEST_AREA_OFFSET + TEST_AREA_SIZE)
#define TEST_AREA_DEVICE	PARTITION_DEVICE(storage_partition)

static const struct device *const flash_dev = TEST_AREA_DEVICE;

ZTEST(flash_preload, test_preload_data)
{
	char word[sizeof(MAGIC_STRING)];
	int rc;

	rc = flash_read(flash_dev, TEST_AREA_OFFSET, &word, sizeof(word));
	zassert_equal(0, rc, "Failed to read flash");
	zassert_mem_equal(MAGIC_STRING, word, sizeof(MAGIC_STRING),
					  "Magic string not present in flash");
}

void *flash_preload_setup(void)
{
	zassert_true(device_is_ready(flash_dev), "flash device not ready");

	return NULL;
}

ZTEST_SUITE(flash_preload, NULL, flash_preload_setup, NULL, NULL, NULL);
