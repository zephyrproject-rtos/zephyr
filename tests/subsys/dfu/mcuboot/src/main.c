/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>

#define BOOT_MAGIC_VAL_W0 0xf395c277
#define BOOT_MAGIC_VAL_W1 0x7fefd260
#define BOOT_MAGIC_VAL_W2 0x0f505235
#define BOOT_MAGIC_VAL_W3 0x8079b62c
#define BOOT_MAGIC_VALUES {BOOT_MAGIC_VAL_W0, BOOT_MAGIC_VAL_W1,\
			   BOOT_MAGIC_VAL_W2, BOOT_MAGIC_VAL_W3 }

void test_bank_erase(void)
{
	const struct flash_area *fa;
	uint32_t temp;
	uint32_t temp2 = 0x5a5a5a5a;
	off_t offs;
	int ret;

	ret = flash_area_open(FLASH_AREA_ID(image_1), &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	for (offs = 0; offs < fa->fa_size; offs += sizeof(temp)) {
		ret = flash_area_read(fa, offs, &temp, sizeof(temp));
		zassert_true(ret == 0, "Reading from flash");
		if (temp == 0xFFFFFFFF) {
			ret = flash_area_write(fa, offs, &temp2, sizeof(temp));
			zassert_true(ret == 0, "Writing to flash");
		}
	}

	zassert(boot_erase_img_bank(FLASH_AREA_ID(image_1)) == 0,
		"pass", "fail");

	for (offs = 0; offs < fa->fa_size; offs += sizeof(temp)) {
		ret = flash_area_read(fa, offs, &temp, sizeof(temp));
		zassert_true(ret == 0, "Reading from flash");
		zassert(temp == 0xFFFFFFFF, "pass", "fail");
	}
}

void test_request_upgrade(void)
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

	ret = flash_area_open(FLASH_AREA_ID(image_1), &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	zassert(boot_request_upgrade(false) == 0, "pass", "fail");

	ret = flash_area_read(fa, fa->fa_size - sizeof(expectation),
			      &readout, sizeof(readout));
	zassert_true(ret == 0, "Read from flash");

	zassert(memcmp(expectation, readout, sizeof(expectation)) == 0,
		"pass", "fail");

	zassert(boot_erase_img_bank(FLASH_AREA_ID(image_1)) == 0, "pass", "fail");

	zassert(boot_request_upgrade(true) == 0, "pass", "fail");

	ret = flash_area_read(fa, fa->fa_size - sizeof(expectation),
			      &readout, sizeof(readout));
	zassert_true(ret == 0, "Read from flash");

	zassert(memcmp(&expectation[2], &readout[2], sizeof(expectation) -
		       2 * sizeof(expectation[0])) == 0, "pass", "fail");

	zassert_equal(1, readout[0] & 0xff, "confirmation error");
}

void test_write_confirm(void)
{
	const uint32_t img_magic[4] = BOOT_MAGIC_VALUES;
	uint32_t readout[ARRAY_SIZE(img_magic)];
	uint8_t flag[BOOT_MAX_ALIGN];
	const struct flash_area *fa;
	int ret;

	flag[0] = 0x01;
	memset(&flag[1], 0xff, sizeof(flag) - 1);

	ret = flash_area_open(FLASH_AREA_ID(image_0), &fa);
	if (ret) {
		printf("Flash driver was not found!\n");
		return;
	}

	zassert(boot_erase_img_bank(FLASH_AREA_ID(image_0)) == 0,
		"pass", "fail");

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

	ret = boot_write_img_confirmed();
	zassert(ret == 0, "pass", "fail (%d)", ret);

	ret = flash_area_read(fa, fa->fa_size - 24, readout,
			      sizeof(readout[0]));
	zassert_true(ret == 0, "Read from flash");

	zassert_equal(1, readout[0] & 0xff, "confirmation error");
}

void test_main(void)
{
	ztest_test_suite(test_mcuboot_interface,
			 ztest_unit_test(test_bank_erase),
			 ztest_unit_test(test_request_upgrade),
			 ztest_unit_test(test_write_confirm));
	ztest_run_test_suite(test_mcuboot_interface);
}
