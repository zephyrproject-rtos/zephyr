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
	int ret;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);

	for (offs = FLASH_AREA_IMAGE_1_OFFSET;
	     offs <= FLASH_AREA_IMAGE_1_OFFSET + FLASH_AREA_IMAGE_1_SIZE;
	     offs += sizeof(temp)) {
		ret = flash_read(flash_dev, offs, &temp, sizeof(temp));
		zassert_true(ret == 0, "Reading from flash");
		if (temp != 0xFFFFFFFF) {
			ret = flash_write_protection_set(flash_dev, false);
			zassert_true(ret == 0, "Disabling flash protection");

			ret = flash_write(flash_dev, offs, &temp2,
					  sizeof(temp));
			zassert_true(ret == 0, "Writing to flash");

			ret = flash_write_protection_set(flash_dev, true);
			zassert_true(ret == 0, "Enabling flash protection");
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
		ret = flash_read(flash_dev, offs, &temp, sizeof(temp));
		zassert_true(ret == 0, "Reading from flash");
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
	int ret;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);

	zassert(boot_request_upgrade(false) == 0, "pass", "fail");

	ret = flash_read(flash_dev, FLASH_AREA_IMAGE_1_OFFSET +
			 FLASH_AREA_IMAGE_1_SIZE - sizeof(expectation),
			 &readout, sizeof(readout));
	zassert_true(ret == 0, "Read from flash");

	zassert(memcmp(expectation, readout, sizeof(expectation)) == 0,
		"pass", "fail");

	boot_erase_img_bank(FLASH_AREA_IMAGE_1_OFFSET);

	zassert(boot_request_upgrade(true) == 0, "pass", "fail");

	ret = flash_read(flash_dev, FLASH_AREA_IMAGE_1_OFFSET +
			 FLASH_AREA_IMAGE_1_SIZE - sizeof(expectation),
			 &readout, sizeof(readout));
	zassert_true(ret == 0, "Read from flash");

	zassert(memcmp(&expectation[2], &readout[2], sizeof(expectation) -
		       2 * sizeof(expectation[0])) == 0, "pass", "fail");

	zassert_equal(1, readout[0] & 0xff, "confirmation error");
}

void test_write_confirm(void)
{
	const u32_t img_magic[4] = BOOT_MAGIC_VALUES;
	u32_t readout[ARRAY_SIZE(img_magic)];
	struct device *flash_dev;
	int ret;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);

	ret = flash_read(flash_dev, FLASH_AREA_IMAGE_0_OFFSET +
			 FLASH_AREA_IMAGE_0_SIZE - sizeof(img_magic), &readout,
			 sizeof(img_magic));
	zassert_true(ret == 0, "Read from flash");

	if (memcmp(img_magic, readout, sizeof(img_magic)) != 0) {
		ret = flash_write_protection_set(flash_dev, false);
		zassert_true(ret == 0, "Disable flash protection");

		ret = flash_write(flash_dev, FLASH_AREA_IMAGE_0_OFFSET +
				  FLASH_AREA_IMAGE_0_SIZE - 16,
				  img_magic, 16);
		zassert_true(ret == 0, "Write to flash");

		ret = flash_write_protection_set(flash_dev, true);
		zassert_true(ret == 0, "Enable flash protection");
	}

	zassert(boot_write_img_confirmed() == 0, "pass", "fail");

	ret = flash_read(flash_dev, FLASH_AREA_IMAGE_0_OFFSET +
			 FLASH_AREA_IMAGE_0_SIZE - 24, readout,
			 sizeof(readout[0]));
	zassert_true(ret == 0, "Read from flash");

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
