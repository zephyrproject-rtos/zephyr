/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <device.h>
#include <stdio.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(test_flash);

#define FLASH_TEST_REGION_OFFSET 0x3F0000
#define FLASH_SECTOR_SIZE        0x10000
#define TEST_DATA_BYTE_0         0x4f
#define TEST_DATA_BYTE_1         0x4a
#define TEST_DATA_LEN            128

int flash_region_is_empty(u32_t off, void *dst, u32_t len)
{
	u8_t i;
	u8_t *u8dst;
	int rc;
	const struct flash_area *fap;

	rc = flash_area_open(DT_FLASH_AREA_IMAGE_SCRATCH_ID, &fap);
	if (rc != 0) {
		LOG_ERR("SPI flash area open failed!\n");
		return -1;
	}

	rc = flash_area_read(fap, off - fap->fa_off, dst, len);
	if (rc) {
		LOG_ERR("SPI flash efailed!\n");
		return -1;
	}

	for (i = 0U, u8dst = (uint8_t *)dst; i < len; i++) {
		if (u8dst[i] != 0xFF) {
			flash_area_close(fap);
			return 0;
		}
	}

	flash_area_close(fap);

	return 1;
}

void test_flash(void)
{
	struct device *flash_dev;
	u8_t buf[TEST_DATA_LEN];
	u32_t magic[4];
	int i;

	flash_dev = device_get_binding(DT_INST_0_JEDEC_SPI_NOR_LABEL);

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

	if (flash_region_is_empty(
			FLASH_TEST_REGION_OFFSET - 16, magic, 16) == 1) {
		LOG_INF("   Flash region is empty. Good!!\n");
	} else {
		LOG_ERR("   Flash region is not empty!!\n");
	}
}
