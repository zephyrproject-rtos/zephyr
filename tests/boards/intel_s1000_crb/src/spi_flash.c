/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <flash.h>
#include <device.h>
#include <stdio.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(test_flash);

#define FLASH_TEST_REGION_OFFSET 0x3F0000
#define FLASH_SECTOR_SIZE        0x10000
#define TEST_DATA_BYTE_0         0x4f
#define TEST_DATA_BYTE_1         0x4a
#define TEST_DATA_LEN            128

void test_flash(void)
{
	struct device *flash_dev;
	u8_t buf[TEST_DATA_LEN];
	int i;

	flash_dev = device_get_binding(CONFIG_SPI_NOR_DRV_NAME);

	if (!flash_dev) {
		LOG_ERR("SPI flash driver was not found!\n");
		return;
	}

	LOG_INF("SPI flash driver was found!\n");

	flash_write_protection_set(flash_dev, false);

	if (flash_erase(flash_dev,
			FLASH_TEST_REGION_OFFSET,
			FLASH_SECTOR_SIZE) != 0) {
		LOG_ERR("   Flash erase failed!\n");
	} else {
		LOG_INF("   Flash erase succeeded!\n");
	}

	buf[0] = TEST_DATA_BYTE_0;
	buf[1] = TEST_DATA_BYTE_1;

	for (i = 2 ; i < TEST_DATA_LEN; i++) {
		buf[i] = 0x63;
	}

	LOG_INF("   Attempted to write %x %x\n", buf[0], buf[1]);
	if (flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
	    TEST_DATA_LEN) != 0) {
		LOG_ERR("   Flash write failed!\n");
		return;
	}

	if (flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
	    TEST_DATA_LEN) != 0) {
		LOG_ERR("   Flash read failed!\n");
		return;
	}

	if ((buf[0] == TEST_DATA_BYTE_0) && (buf[1] == TEST_DATA_BYTE_1)) {
		LOG_INF("   Data read matches with data written. Good!!\n");
	} else {
		LOG_ERR("   Data read does not match with data written!!\n");
	}
}
