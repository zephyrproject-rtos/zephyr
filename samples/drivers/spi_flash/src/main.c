/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/flash.h>
#include <device.h>
#include <stdio.h>

#if (CONFIG_SPI_FLASH_W25QXXDV - 0)
/* NB: W25Q16DV is a JEDEC spi-nor device, but has a separate driver. */
#define FLASH_DEVICE CONFIG_SPI_FLASH_W25QXXDV_DRV_NAME
#define FLASH_NAME "W25QXXDV"
#elif (CONFIG_SPI_NOR - 0) || defined(DT_INST_0_JEDEC_SPI_NOR_LABEL)
#define FLASH_DEVICE DT_INST_0_JEDEC_SPI_NOR_LABEL
#define FLASH_NAME "JEDEC SPI-NOR"
#else
#error Unsupported flash driver
#endif

#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE        4096
#define TEST_DATA_BYTE_0         0x55
#define TEST_DATA_BYTE_1         0xaa
#define TEST_DATA_LEN            2

void main(void)
{
	struct device *flash_dev;
	u8_t buf[TEST_DATA_LEN];

	printf("\n" FLASH_NAME " SPI flash testing\n");
	printf("==========================\n");

	flash_dev = device_get_binding(FLASH_DEVICE);

	if (!flash_dev) {
		printf("SPI flash driver %s was not found!\n", FLASH_DEVICE);
		return;
	}

	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	printf("\nTest 1: Flash erase\n");
	flash_write_protection_set(flash_dev, false);
	if (flash_erase(flash_dev,
			FLASH_TEST_REGION_OFFSET,
			FLASH_SECTOR_SIZE) != 0) {
		printf("   Flash erase failed!\n");
	} else {
		printf("   Flash erase succeeded!\n");
	}

	printf("\nTest 2: Flash write\n");
	flash_write_protection_set(flash_dev, false);

	buf[0] = TEST_DATA_BYTE_0;
	buf[1] = TEST_DATA_BYTE_1;
	printf("   Attempted to write %x %x\n", buf[0], buf[1]);
	if (flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
	    TEST_DATA_LEN) != 0) {
		printf("   Flash write failed!\n");
		return;
	}

	if (flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
	    TEST_DATA_LEN) != 0) {
		printf("   Flash read failed!\n");
		return;
	}
	printf("   Data read %x %x\n", buf[0], buf[1]);

	if ((buf[0] == TEST_DATA_BYTE_0) && (buf[1] == TEST_DATA_BYTE_1)) {
		printf("   Data read matches with data written. Good!!\n");
	} else {
		printf("   Data read does not match with data written!!\n");
	}
}
