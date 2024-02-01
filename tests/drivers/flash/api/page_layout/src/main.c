/*
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>

#define TEST_NODE DT_ALIAS(dut)

#if !DT_NODE_EXISTS(TEST_NODE)
#error "Unsupported configuration"
#endif

static const struct device *const fldev = DEVICE_DT_GET(DT_PARENT(TEST_NODE));
static const size_t flsize = DT_REG_SIZE(TEST_NODE);

static void *flash_page_layout_setup(void)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		k_object_access_grant(fldev, k_current_get());
	}

	return NULL;
}

ZTEST_USER(flash_page_layout, test_a_flash_get_page_info)
{
	struct flash_pages_info fpi;
	int rc;

	zassert_true(device_is_ready(fldev));

	fpi.start_offset = 0;
	fpi.size = 0;
	fpi.index = 0U;
	while (fpi.start_offset < flsize) {
		size_t idx = fpi.index;

		rc = flash_get_page_info_by_offs(fldev, fpi.start_offset, &fpi);
		zassert_equal(rc, 0, "flash_get_page_info_by_offs() invalid");
		zassert_equal(fpi.index, idx, "invalid index");
		fpi.start_offset += fpi.size;
		fpi.index++;
	}

	TC_PRINT("Device has %d erase-blocks\n", fpi.index);

	fpi.start_offset += fpi.size;
	rc = flash_get_page_info_by_offs(fldev, fpi.start_offset, &fpi);
	zassert_true(rc == -EINVAL, "flash_get_page_info_by_offs() invalid");

	fpi.start_offset = 0;
	fpi.size = 0;
	fpi.index = 0U;
	while (fpi.start_offset < flsize) {
		off_t offset = fpi.start_offset;

		rc = flash_get_page_info_by_idx(fldev, fpi.index, &fpi);
		zassert_equal(rc, 0, "flash_get_page_info_by_idx() invalid");
		zassert_equal(fpi.start_offset, offset, "invalid offset");
		fpi.start_offset += fpi.size;
		fpi.index++;
	}

	TC_PRINT("Device has %d erase-blocks\n", fpi.index);

	fpi.index++;
	rc = flash_get_page_info_by_idx(fldev, fpi.index, &fpi);
	zassert_true(rc == -EINVAL, "flash_get_page_info_by_idx() invalid");
}

ZTEST_USER(flash_page_layout, test_b_flash_get_page_count)
{
	size_t count;

	zassert_true(device_is_ready(fldev));
	count = flash_get_page_count(fldev);
	zassert_false(count <= 0, "flash_get_page_count() invalid");

	TC_PRINT("Device has %d erase-blocks\n", count);
}

ZTEST_USER(flash_page_layout, test_c_flash_get_erase_region)
{
	struct flash_erase_region region;
	int rc;

	zassert_true(device_is_ready(fldev));

	region.offset = 0;
	while (region.offset < flsize) {
		off_t offset = region.offset;
		size_t rsize, ebsize;

		rc = flash_get_erase_region(fldev, offset, &region);
		zassert_equal(rc, 0, "flash_get_erase_region() invalid");
		zassert_equal(offset, region.offset, "invalid start_offset");
		zassert_false(region.size == 0U, "invalid size");
		zassert_false(region.erase_block_size == 0U, "invalid erase_block_size");

		rsize = region.size;
		ebsize = region.erase_block_size;

		if (rsize > ebsize) {
			rc = flash_get_erase_region(fldev, offset + ebsize, &region);
			zassert_equal(rc, 0, "flash_get_erase_region() invalid");
			zassert_equal(offset, region.offset, "invalid start_offset");
			zassert_equal(rsize, region.size, "invalid size");
		}

		rc = flash_get_erase_region(fldev, offset + 1, &region);
		zassert_equal(rc, 0, "flash_get_erase_region() invalid");
		zassert_equal(offset, region.offset, "invalid start_offset");

		region.offset += region.size;
	}

	rc = flash_get_erase_region(fldev, region.offset, &region);
	zassert_equal(rc, -EINVAL, "flash_get_erase_region() invalid");
}

bool test_cb(const struct flash_pages_info *info, void *data)
{
	size_t *index = (size_t *)data;

	*index = *index + 1;
	if (info->start_offset < flsize) {
		return true;
	}

	return false;
}

ZTEST_USER(flash_page_layout, test_d_flash_page_foreach)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		TC_PRINT("flash_page_foreach is not supported for userspace threads\n");
		return;
	}

	size_t pcnt = 0U;

	zassert_true(device_is_ready(fldev));
	flash_page_foreach(fldev, test_cb, (void *)&pcnt);

	TC_PRINT("Device has %d erase-blocks\n", pcnt);
}

ZTEST_SUITE(flash_page_layout, NULL, flash_page_layout_setup, NULL, NULL, NULL);
