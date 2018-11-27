/*
 * Copyright (c) 2018 Sigma Connectivity.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <flash.h>
#include <gpio.h>
#include <device.h>
#include <stdio.h>

#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE        4096
#define TEST_DATA_BYTE_0         0x55
#define TEST_DATA_BYTE_1         0xaa
#define TEST_DATA_LEN            2

void main(void)
{
	struct device *flash_dev;
	u8_t buf[TEST_DATA_LEN];

	printk("\nMX25R6435F SPI flash testing\n");
	printk("==========================\n");

	flash_dev = device_get_binding(CONFIG_SPI_FLASH_MX25R6435F_DRV_NAME);

	if (!flash_dev) {
		printk("SPI flash driver was not found!\n");
		return;
	}

	/* Write protection needs to be disabled in MX25R6435F flash before
	 * each write or erase. This is because the flash component turns
	 * on write protection automatically after completion of write and
	 * erase operations.
	 */
	printk("\nTest 1: Flash erase\n");
	flash_write_protection_set(flash_dev, false);
	if (flash_erase(flash_dev,
			FLASH_TEST_REGION_OFFSET,
			FLASH_SECTOR_SIZE) != 0) {
		printk("   Flash erase failed!\n");
	} else {
		printk("   Flash erase succeeded!\n");
	}

	printk("\nTest 2: Flash write\n");
	flash_write_protection_set(flash_dev, false);

	buf[0] = TEST_DATA_BYTE_0;
	buf[1] = TEST_DATA_BYTE_1;
	printk("   Attempted to write %x %x\n", buf[0], buf[1]);
	if (flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
			TEST_DATA_LEN) != 0) {
		printk("   Flash write failed!\n");
		return;
	}

	if (flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
		       TEST_DATA_LEN) != 0) {
		printk("   Flash read failed!\n");
		return;
	}
	printk("   Data read %x %x\n", buf[0], buf[1]);

	if ((buf[0] == TEST_DATA_BYTE_0) && (buf[1] == TEST_DATA_BYTE_1)) {
		printk("   Data read matches with data written. Good!!\n");
	} else {
		printk("   Data read does not match with data written!!\n");
	}
}
