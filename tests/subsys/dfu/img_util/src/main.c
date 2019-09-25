/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <storage/flash_map.h>
#include <dfu/flash_img.h>

void test_collecting(void)
{
	const struct flash_area *fa;
	struct flash_img_context ctx;
	u32_t i, j;
	u8_t data[5], temp, k;
	int ret;

	ret = flash_img_init(&ctx);
	zassert_true(ret == 0, "Flash img init");

	ret = flash_area_erase(ctx.flash_area, 0, ctx.flash_area->fa_size);
	zassert_true(ret == 0, "Flash erase");

	zassert(flash_img_bytes_written(&ctx) == 0, "pass", "fail");

	k = 0U;
	for (i = 0U; i < 300; i++) {
		for (j = 0U; j < ARRAY_SIZE(data); j++) {
			data[j] = k++;
		}
		zassert(flash_img_buffered_write(&ctx, data, sizeof(data),
						 false) == 0, "pass", "fail");
	}

	zassert(flash_img_buffered_write(&ctx, data, 0, true) == 0, "pass",
					 "fail");


	ret = flash_area_open(DT_FLASH_AREA_IMAGE_1_ID, &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	k = 0U;
	for (i = 0U; i < 300 * sizeof(data); i++) {
		zassert(flash_area_read(fa, i, &temp, 1) == 0, "pass", "fail");
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
