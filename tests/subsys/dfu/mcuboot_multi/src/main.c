/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/drivers/flash.h>

#define BOOT_MAGIC_VAL_W0 0xf395c277
#define BOOT_MAGIC_VAL_W1 0x7fefd260
#define BOOT_MAGIC_VAL_W2 0x0f505235
#define BOOT_MAGIC_VAL_W3 0x8079b62c
#define BOOT_MAGIC_VALUES {BOOT_MAGIC_VAL_W0, BOOT_MAGIC_VAL_W1,\
			   BOOT_MAGIC_VAL_W2, BOOT_MAGIC_VAL_W3 }

static void _test_request_upgrade_n(uint8_t fa_id, int img_index, int confirmed)
{
	const struct flash_area *fa;
	const uint32_t expectation[6] = {
		0xffffffff,
		0xffffffff,
		BOOT_MAGIC_VAL_W0,
		BOOT_MAGIC_VAL_W1,
		BOOT_MAGIC_VAL_W2,
		BOOT_MAGIC_VAL_W3
	};
	uint32_t readout[ARRAY_SIZE(expectation)];
	int ret;
	struct flash_pages_info page;
	const struct device *sf_dev;

	ret = flash_area_open(fa_id, &fa);
	zassert_true(ret == 0, "can't open the images's flash area.");

	sf_dev = flash_area_get_device(fa);

	ret = flash_get_page_info_by_offs(sf_dev, fa->fa_size - 1, &page);
	ret = flash_erase(sf_dev, page.start_offset, page.size);

	ret = (confirmed) ? BOOT_UPGRADE_PERMANENT : BOOT_UPGRADE_TEST;
	zassert_true(boot_request_upgrade_multi(img_index, ret) == 0,
		     "Can' request the upgrade of the %d. image.", img_index);

	ret = flash_area_read(fa, fa->fa_size - sizeof(expectation),
			      &readout, sizeof(readout));
	zassert_true(ret == 0, "Read from flash");

	if (confirmed) {
		zassert_true(memcmp(&expectation[2], &readout[2],
				    sizeof(expectation) -
				    2 * sizeof(expectation[0])) == 0,
				    "unexpected trailer value");

		zassert_equal(1, readout[0] & 0xff, "confirmation error");
	} else {
		zassert_true(memcmp(expectation, readout,
		sizeof(expectation)) == 0, "unexpected trailer value");
	}
}

ZTEST(mcuboot_multi, test_request_upgrade_multi)
{
	_test_request_upgrade_n(FLASH_AREA_ID(image_1), 0, 0);
	_test_request_upgrade_n(FLASH_AREA_ID(image_3), 1, 1);
}

static void _test_write_confirm_n(uint8_t fa_id, int img_index)
{
	const uint32_t img_magic[4] = BOOT_MAGIC_VALUES;
	uint32_t readout[ARRAY_SIZE(img_magic)];
	uint8_t flag[BOOT_MAX_ALIGN];
	const struct flash_area *fa;
	int ret;
	struct flash_pages_info page;
	const struct device *sf_dev;

	flag[0] = 0x01;
	memset(&flag[1], 0xff, sizeof(flag) - 1);

	ret = flash_area_open(fa_id, &fa);
	zassert_true(ret == 0, "can't open the images's flash area.");

	sf_dev = flash_area_get_device(fa);

	ret = flash_get_page_info_by_offs(sf_dev, fa->fa_size - 1, &page);
	zassert_true(ret == 0, "can't get the trailer's flash page info.");

	ret = flash_erase(sf_dev, page.start_offset, page.size);
	zassert_true(ret == 0, "can't erase the trailer flash page.");

	ret = flash_area_read(fa, fa->fa_size - sizeof(img_magic),
			      &readout, sizeof(img_magic));
	zassert_true(ret == 0, "Read from flash");

	if (memcmp(img_magic, readout, sizeof(img_magic)) != 0) {
		ret = flash_area_write(fa, fa->fa_size - 16,
				       img_magic, 16);
		zassert_true(ret == 0, "Write to flash");
	}

	/* set copy-done flag */
	ret = flash_area_write(fa, fa->fa_size - 32, &flag, sizeof(flag));
	zassert_true(ret == 0, "Write to flash");

	ret = boot_write_img_confirmed_multi(img_index);
	zassert(ret == 0, "pass", "fail (%d)", ret);

	ret = flash_area_read(fa, fa->fa_size - 24, readout,
			      sizeof(readout[0]));
	zassert_true(ret == 0, "Read from flash");

	zassert_equal(1, readout[0] & 0xff, "confirmation error");
}

ZTEST(mcuboot_multi, test_write_confirm_multi)
{
	_test_write_confirm_n(FLASH_AREA_ID(image_0), 0);
	_test_write_confirm_n(FLASH_AREA_ID(image_2), 1);
}

ZTEST_SUITE(mcuboot_multi, NULL, NULL, NULL, NULL, NULL);
