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
#define SLOT1_PARTITION_SIZE	FIXED_PARTITION_SIZE(SLOT1_PARTITION)

#define FLASH_AREA_COPY_SIZE	MIN((SLOT1_PARTITION_SIZE / 2), 128)

extern int flash_map_entries;
struct flash_sector fs_sectors[2048];

ZTEST(flash_map, test_flash_area_disabled_device)
{
	/* The test checks if Flash Map will report partition
	 * non-existend if it is disabled, but it also assumes that
	 * disabled parition will have an ID generated.
	 * Custom partition maps may not be generating entries for
	 * disabled partitions nor identifiers, which makes the
	 * test fail with custom partition manager, for no real reason.
	 */
#if defined(CONFIG_TEST_FLASH_MAP_DISABLED_PARTITIONS)
	const struct flash_area *fa;
	int rc;

	/* Test that attempting to open a disabled flash area fails */
	rc = flash_area_open(FIXED_PARTITION_ID(disabled_a), &fa);
	zassert_equal(rc, -ENOENT, "Open did not fail");
	rc = flash_area_open(FIXED_PARTITION_ID(disabled_a_a), &fa);
	zassert_equal(rc, -ENOENT, "Open did not fail");
	rc = flash_area_open(FIXED_PARTITION_ID(disabled_a_b), &fa);
	zassert_equal(rc, -ENOENT, "Open did not fail");
	rc = flash_area_open(FIXED_PARTITION_ID(disabled_b), &fa);
	zassert_equal(rc, -ENOENT, "Open did not fail");
	rc = flash_area_open(FIXED_PARTITION_ID(disabled_b_a), &fa);
	zassert_equal(rc, -ENOENT, "Open did not fail");
	rc = flash_area_open(FIXED_PARTITION_ID(disabled_b_b), &fa);
	zassert_equal(rc, -ENOENT, "Open did not fail");

	/* Note lack of tests for FIXED_PARTITION(...) instantiation,
	 * because this macro will fail, at compile time, if node does not
	 * exist or is disabled.
	 */
#else
	ztest_test_skip();
#endif
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
	/* DTS node macros, for accessing fixed partitions, are only available
	 * for DTS based partitions; custom flash map may define partition outside
	 * of DTS definition, making the NODE macros fail to evaluate.
	 */
#if defined(CONFIG_TEST_FLASH_MAP_NODE_MACROS)
	/* Test against changes in API */
	zassert_equal(FIXED_PARTITION_NODE_OFFSET(SLOT1_PARTITION_NODE),
		DT_REG_ADDR(SLOT1_PARTITION_NODE));
	zassert_equal(FIXED_PARTITION_NODE_SIZE(SLOT1_PARTITION_NODE),
		DT_REG_SIZE(SLOT1_PARTITION_NODE));
	zassert_equal(FIXED_PARTITION_NODE_DEVICE(SLOT1_PARTITION_NODE),
		DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(SLOT1_PARTITION_NODE)));

	/* Taking by node and taking by label should give same device */
	zassert_equal(FIXED_PARTITION_BY_NODE(DT_NODELABEL(SLOT1_PARTITION)),
		      FIXED_PARTITION(SLOT1_PARTITION));
#else
	ztest_test_skip();
#endif
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
		     (int)(i + fa->fa_off));

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
		     (int)(i + fa->fa_off));
}

ZTEST(flash_map, test_flash_area_copy)
{
	const struct flash_area *fa;
	uint8_t src_buf[FLASH_AREA_COPY_SIZE], dst_buf[FLASH_AREA_COPY_SIZE],
		copy_buf[32];
	int rc;

	/* Get source and destination flash areas */
	fa = FIXED_PARTITION(SLOT1_PARTITION);

	/* First erase the area so it's ready for use. */
	rc = flash_area_erase(fa, 0, fa->fa_size);
	zassert_true(rc == 0, "flash area erase fail");

	/* Fill source area with test data */
	memset(src_buf, 0xAB, sizeof(src_buf));
	rc = flash_area_write(fa, 0, src_buf, sizeof(src_buf));
	zassert_true(rc == 0, "Failed to write to source flash area");

	/* Perform the copy operation */
	rc = flash_area_copy(fa, 0, fa, FLASH_AREA_COPY_SIZE, sizeof(src_buf), copy_buf,
			     sizeof(copy_buf));
	zassert_true(rc == 0, "flash_area_copy failed");

	/* Verify the copied data */
	rc = flash_area_read(fa, FLASH_AREA_COPY_SIZE, dst_buf, sizeof(dst_buf));
	zassert_true(rc == 0, "Failed to read from destination flash area");
	zassert_mem_equal(src_buf, dst_buf, sizeof(src_buf), "Data mismatch after copy");
}

ZTEST(flash_map, test_parameter_overflows)
{
	const struct flash_area *fa;
	uint8_t dst_buf[FLASH_AREA_COPY_SIZE];
	int rc;

	fa = FIXED_PARTITION(SLOT1_PARTITION);
	/* -1 cast to size_t gives us max size_t value, added to offset of 1,
	 * it will overflow to 0.
	 */
	rc = flash_area_read(fa, 1, dst_buf, (size_t)(-1));
	zassert_equal(rc, -EINVAL, "1: Overflow should have been detected");
	/* Here we have offset 1 below size of area, with added max size_t
	 * it upper bound of read range should overflow to:
	 * (max(size_t) + fa->fa_size - 1) mod (max(size_t)) == fa->fa_size - 2
	 */
	rc = flash_area_read(fa, fa->fa_size - 1, dst_buf, (size_t)(-1));
	zassert_equal(rc, -EINVAL, "2: Overflow should have been detected");
}

ZTEST_SUITE(flash_map, NULL, NULL, NULL, NULL, NULL);
