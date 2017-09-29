/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <flash.h>
#include <dfu/mcuboot.h>

#define BOOT_MAGIC_VAL_W0 0xf395c277
#define BOOT_MAGIC_VAL_W1 0x7fefd260
#define BOOT_MAGIC_VAL_W2 0x0f505235
#define BOOT_MAGIC_VAL_W3 0x8079b62c
#define BOOT_MAGIC_VALUES {BOOT_MAGIC_VAL_W0, BOOT_MAGIC_VAL_W1,\
			   BOOT_MAGIC_VAL_W2, BOOT_MAGIC_VAL_W3 }

void test_bank_erase(void)
{
	struct device *flash_dev;
	u32_t temp;
	u32_t temp2 = 0x5a;
	off_t offs;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);

	for (offs = FLASH_AREA_IMAGE_1_OFFSET;
	     offs <= FLASH_AREA_IMAGE_1_OFFSET + FLASH_AREA_IMAGE_1_SIZE;
	     offs += sizeof(temp)) {
		flash_read(flash_dev, offs, &temp, sizeof(temp));
		if (temp != 0xFFFFFFFF) {
			flash_write_protection_set(flash_dev, false);
			flash_write(flash_dev, offs, &temp2, sizeof(temp));
			flash_write_protection_set(flash_dev, true);
		}
	}

	zassert(boot_erase_img_bank(FLASH_AREA_IMAGE_1_OFFSET) == 0,
		"pass", "fail");

	if (!flash_dev) {
		printf("Nordic nRF5 flash driver was not found!\n");
		return;
	}

	for (offs = FLASH_AREA_IMAGE_1_OFFSET;
	     offs <= FLASH_AREA_IMAGE_1_OFFSET + FLASH_AREA_IMAGE_1_SIZE;
	     offs += sizeof(temp)) {
		flash_read(flash_dev, offs, &temp, sizeof(temp));
		zassert(temp == 0xFFFFFFFF, "pass", "fail");
	}
}

void test_request_upgrade(void)
{
	struct device *flash_dev;

	const u32_t expectation[6] = {
		0xffffffff,
		0xffffffff,
		BOOT_MAGIC_VAL_W0,
		BOOT_MAGIC_VAL_W1,
		BOOT_MAGIC_VAL_W2,
		BOOT_MAGIC_VAL_W3
	};

	u32_t readout[ARRAY_SIZE(expectation)];

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);

	zassert(boot_request_upgrade(false) == 0, "pass", "fail");

	flash_read(flash_dev, FLASH_AREA_IMAGE_1_OFFSET +
		   FLASH_AREA_IMAGE_1_SIZE - sizeof(expectation), &readout,
		   sizeof(readout));

	zassert(memcmp(expectation, readout, sizeof(expectation)) == 0,
		"pass", "fail");

	boot_erase_img_bank(FLASH_AREA_IMAGE_1_OFFSET);

	zassert(boot_request_upgrade(true) == 0, "pass", "fail");

	flash_read(flash_dev, FLASH_AREA_IMAGE_1_OFFSET +
		   FLASH_AREA_IMAGE_1_SIZE - sizeof(expectation), &readout,
		   sizeof(readout));

	zassert(memcmp(&expectation[2], &readout[2], sizeof(expectation) -
		       2 * sizeof(expectation[0])) == 0, "pass", "fail");

	zassert_equal(1, readout[0] & 0xff, "confirmation error");
}

void test_write_confirm(void)
{
	const u32_t img_magic[4] = BOOT_MAGIC_VALUES;

	struct device *flash_dev;

	u32_t readout[ARRAY_SIZE(img_magic)];

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);

	flash_read(flash_dev, FLASH_AREA_IMAGE_0_OFFSET +
		   FLASH_AREA_IMAGE_0_SIZE - sizeof(img_magic), &readout,
		   sizeof(img_magic));

	if (memcmp(img_magic, readout, sizeof(img_magic)) != 0) {
		flash_write_protection_set(flash_dev, false);
		flash_write(flash_dev, FLASH_AREA_IMAGE_0_OFFSET +
			    FLASH_AREA_IMAGE_0_SIZE - 16, img_magic, 16);
		flash_write_protection_set(flash_dev, true);
	}

	zassert(boot_write_img_confirmed() == 0, "pass", "fail");

	flash_read(flash_dev, FLASH_AREA_IMAGE_0_OFFSET +
		   FLASH_AREA_IMAGE_0_SIZE - 24, readout,
		   sizeof(readout[0]));

	zassert_equal(1, readout[0] & 0xff, "confirmation error");
}

void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_mcuboot_interface,
			 ztest_unit_test(test_bank_erase),
			 ztest_unit_test(test_request_upgrade),
			 ztest_unit_test(test_write_confirm));
	ztest_run_test_suite(test_mcuboot_interface);
}
