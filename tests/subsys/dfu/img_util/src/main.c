/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <flash.h>
#include <dfu/flash_img.h>

void test_collecting(void)
{
	struct device *flash_dev;
	struct flash_img_context ctx;
	u32_t i, j;
	u8_t data[5], temp, k;

	flash_dev = device_get_binding(FLASH_DEV_NAME);

	flash_write_protection_set(flash_dev, false);
	flash_erase(flash_dev, FLASH_AREA_IMAGE_1_OFFSET,
		    FLASH_AREA_IMAGE_1_SIZE);
	flash_write_protection_set(flash_dev, true);

	flash_img_init(&ctx, flash_dev);
	zassert(flash_img_bytes_written(&ctx) == 0, "pass", "fail");

	k = 0;
	for (i = 0; i < 300; i++) {
		for (j = 0; j < ARRAY_SIZE(data); j++) {
			data[j] = k++;
		}
		zassert(flash_img_buffered_write(&ctx, data, sizeof(data),
						 false) == 0, "pass", "fail");
	}

	zassert(flash_img_buffered_write(&ctx, data, 0, true) == 0, "pass",
					 "fail");

	k = 0;
	for (i = 0; i < 300 * sizeof(data); i++) {
		zassert(flash_read(flash_dev, FLASH_AREA_IMAGE_1_OFFSET + i,
				   &temp, 1) == 0, "pass", "fail");
		zassert(temp == k, "pass", "fail");
		k++;
	}
}

void test_main(void)
{
	ztest_test_suite(test_util,
			ztest_unit_test(test_collecting));
	ztest_run_test_suite(test_util);
}
