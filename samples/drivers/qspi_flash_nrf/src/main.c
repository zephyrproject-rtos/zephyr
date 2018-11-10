/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <flash.h>
#include <misc/printk.h>
#include <zephyr.h>

#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE 0x1000
#define TEST_DATA_BYTE_0 0x55
#define TEST_DATA_BYTE_1 0xaa
#define TEST_DATA_BYTE_2 0x55
#define TEST_DATA_BYTE_3 0xaa
#define TEST_DATA_LEN 4

void main(void)
{
	struct device *flash_dev;
	u8_t buf[TEST_DATA_LEN];

	printk("\nSPI flash testing\n");
	printk("=================\n");

	flash_dev = device_get_binding(DT_SERIAL_FLASH_DEV_NAME);

	if (!flash_dev) {
		printk("SPI flash driver was not found!\n");
		return;
	}

	/* Write protection needs to be disabled in w25qxxdv flash before
	 * each write or erase. This is because the flash component turns
	 * on write protection automatically after completion of write and
	 * erase operations.
	 */
	printk("\nTest 1: Flash erase\n");
	flash_write_protection_set(flash_dev, false);
	if (flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET,
			FLASH_SECTOR_SIZE) != 0) {
		printk("   Flash erase failed!\n");
	} else   {
		printk("   Flash erase succeeded!\n");
	}

	printk("\nTest 2: Flash write\n");
	flash_write_protection_set(flash_dev, false);

	buf[0] = TEST_DATA_BYTE_0;
	buf[1] = TEST_DATA_BYTE_1;
	buf[2] = TEST_DATA_BYTE_2;
	buf[3] = TEST_DATA_BYTE_3;
	printk("   Attempted to write %x %x %x %x\n", buf[0], buf[1], buf[2],
	       buf[3]);
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

	if ((buf[0] == TEST_DATA_BYTE_0) && (buf[1] == TEST_DATA_BYTE_1) &&
	    (buf[2] == TEST_DATA_BYTE_2) && (buf[3] == TEST_DATA_BYTE_3)) {
		printk("   Data read matches with data written. Good!!\n");
	} else   {
		printk("   Data read does not match with data written!!\n");
	}
}
