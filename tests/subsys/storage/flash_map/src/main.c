/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#define SLOT1_PARTITION		slot1_partition
#define SLOT1_PARTITION_ID	FIXED_PARTITION_ID(SLOT1_PARTITION)
#define SLOT1_PARTITION_DEV	FIXED_PARTITION_DEVICE(SLOT1_PARTITION)
#define SLOT1_PARTITION_NODE	DT_NODELABEL(SLOT1_PARTITION)
#define SLOT1_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(SLOT1_PARTITION)

extern int flash_map_entries;
struct flash_sector fs_sectors[1024];

ZTEST(flash_map, test_flash_area_disabled_device)
{
	const struct flash_area *fa;
	int rc;

	/* Test that attempting to open a disabled flash area fails */
	rc = flash_area_open(FIXED_PARTITION_ID(disabled_a), &fa);
	zassert_equal(rc, -ENOENT, "Open did not fail");
	rc = flash_area_open(FIXED_PARTITION_ID(disabled_b), &fa);
	zassert_equal(rc, -ENOENT, "Open did not fail");
}

ZTEST(flash_map, test_flash_area_device_is_ready)
{
	const struct flash_area no_dev = {
		.fa_dev = NULL,
	};

	zassert_false(flash_area_device_is_ready(NULL));
	zassert_false(flash_area_device_is_ready(&no_dev));
	/* The below just assumes that tests are executed so late that
	 * all devices are already initialized and ready.
	 */
	zassert_true(flash_area_device_is_ready(
			FIXED_PARTITION(SLOT1_PARTITION)));
}

static void layout_match(const struct device *flash_dev, uint32_t sec_cnt)
{
	off_t off = 0;
	int i;

	/* For each reported sector, check if it corresponds to real page on device */
	for (i = 0; i < sec_cnt; ++i) {
		struct flash_pages_info fpi;

		zassert_ok(
			flash_get_page_info_by_offs(flash_dev, SLOT1_PARTITION_OFFSET + off, &fpi));
		/* Offset of page taken directly from device corresponds to offset
		 * within flash area
		 */
		zassert_equal(fpi.start_offset, fs_sectors[i].fs_off + SLOT1_PARTITION_OFFSET);
		zassert_equal(fpi.size, fs_sectors[i].fs_size);
		off += fs_sectors[i].fs_size;
	}
}

/**
 * @brief Test flash_area_get_sectors()
 */
ZTEST(flash_map, test_flash_area_get_sectors)
{
	const struct flash_area *fa;
	const struct device *flash_dev_a = SLOT1_PARTITION_DEV;
	uint32_t sec_cnt;
	int rc;

	fa = FIXED_PARTITION(SLOT1_PARTITION);

	zassert_true(flash_area_device_is_ready(fa));

	zassert_true(device_is_ready(flash_dev_a));

	/* Device obtained by label should match the one from fa object */
	zassert_equal(fa->fa_dev, flash_dev_a, "Device for slot1_partition do not match");

	memset(&fs_sectors[0], 0, sizeof(fs_sectors));

	sec_cnt = ARRAY_SIZE(fs_sectors);
	rc = flash_area_get_sectors(SLOT1_PARTITION_ID, &sec_cnt, fs_sectors);
	zassert_true(rc == 0, "flash_area_get_sectors failed");

	layout_match(flash_dev_a, sec_cnt);
}

ZTEST(flash_map, test_flash_area_sectors)
{
	const struct flash_area *fa;
	uint32_t sec_cnt;
	int rc;
	const struct device *flash_dev_a = SLOT1_PARTITION_DEV;

	fa = FIXED_PARTITION(SLOT1_PARTITION);

	zassert_true(flash_area_device_is_ready(fa));

	zassert_true(device_is_ready(flash_dev_a));

	/* Device obtained by label should match the one from fa object */
	zassert_equal(fa->fa_dev, flash_dev_a, "Device for slot1_partition do not match");

	sec_cnt = ARRAY_SIZE(fs_sectors);
	rc = flash_area_sectors(fa, &sec_cnt, fs_sectors);
	zassert_true(rc == 0, "flash_area_get_sectors failed");

	layout_match(flash_dev_a, sec_cnt);
}

ZTEST(flash_map, test_flash_area_erased_val)
{
	const struct flash_parameters *param;
	const struct flash_area *fa;
	uint8_t val;

	fa = FIXED_PARTITION(SLOT1_PARTITION);

	val = flash_area_erased_val(fa);

	param = flash_get_parameters(fa->fa_dev);

	zassert_equal(param->erase_value, val,
		      "value different than the flash erase value");
}

ZTEST(flash_map, test_fixed_partition_node_macros)
{
	/* Test against changes in API */
	zassert_equal(FIXED_PARTITION_NODE_OFFSET(SLOT1_PARTITION_NODE),
		DT_REG_ADDR(SLOT1_PARTITION_NODE));
	zassert_equal(FIXED_PARTITION_NODE_SIZE(SLOT1_PARTITION_NODE),
		DT_REG_SIZE(SLOT1_PARTITION_NODE));
	zassert_equal(FIXED_PARTITION_NODE_DEVICE(SLOT1_PARTITION_NODE),
		DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(SLOT1_PARTITION_NODE)));
}

ZTEST(flash_map, test_flash_area_erase_and_flatten)
{
	int i;
	bool erased = true;
	int rc;
	const struct flash_area *fa;
	const struct device *flash_dev;

	fa = FIXED_PARTITION(SLOT1_PARTITION);

	/* First erase the area so it's ready for use. */
	flash_dev = flash_area_get_device(fa);

	rc = flash_erase(flash_dev, fa->fa_off, fa->fa_size);
	zassert_true(rc == 0, "flash area erase fail");

	rc = flash_fill(flash_dev, 0xaa, fa->fa_off, fa->fa_size);
	zassert_true(rc == 0, "flash device fill fail");

	rc = flash_area_erase(fa, 0, fa->fa_size);
	zassert_true(rc == 0, "flash area erase fail");

	TC_PRINT("Flash area info:\n");
	TC_PRINT("\tpointer:\t %p\n", &fa);
	TC_PRINT("\toffset:\t %ld\n", (long)fa->fa_off);
	TC_PRINT("\tsize:\t %ld\n", (long)fa->fa_size);

	/* we work under assumption that flash_fill is working and tested */
	i = 0;
	while (erased && i < fa->fa_size) {
		uint8_t buf[32];
		int chunk = MIN(sizeof(buf), fa->fa_size - i);

		rc = flash_read(flash_dev, fa->fa_off + i, buf, chunk);
		zassert_equal(rc, 0, "Unexpected read fail with error %d", rc);
		for (int ii = 0; ii < chunk; ++ii, ++i) {
			if ((uint8_t)buf[ii] != (uint8_t)flash_area_erased_val(fa)) {
				erased = false;
				break;
			}
		}
	}
	zassert_true(erased, "Erase failed at dev abosolute offset index %d",
		     i + fa->fa_off);

	rc = flash_fill(flash_dev, 0xaa, fa->fa_off, fa->fa_size);
	zassert_true(rc == 0, "flash device fill fail");

	rc = flash_area_flatten(fa, 0, fa->fa_size);

	erased = true;
	i = 0;
	while (erased && i < fa->fa_size) {
		uint8_t buf[32];
		int chunk = MIN(sizeof(buf), fa->fa_size - i);

		rc = flash_read(flash_dev, fa->fa_off + i, buf, chunk);
		zassert_equal(rc, 0, "Unexpected read fail with error %d", rc);
		for (int ii = 0; ii < chunk; ++ii, ++i) {
			if ((uint8_t)buf[ii] != (uint8_t)flash_area_erased_val(fa)) {
				erased = false;
				break;
			}
		}
	}
	zassert_true(erased, "Flatten/Erase failed at dev absolute offset %d",
		     i + fa->fa_off);
}

ZTEST_SUITE(flash_map, NULL, NULL, NULL, NULL, NULL);
