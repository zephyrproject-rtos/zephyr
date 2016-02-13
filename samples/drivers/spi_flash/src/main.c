/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <flash.h>
#include <device.h>
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE        4096
#define TEST_DATA_BYTE_0         0x55
#define TEST_DATA_BYTE_1         0xaa
#define TEST_DATA_LEN            2

void main(void)
{
	PRINT("SPI flash testing!\n");

	struct device *dev;
	uint8_t buf[TEST_DATA_LEN];

	dev = device_get_binding("W25QXXDV");

	if (!dev) {
		PRINT("SPI flash driver was not found!\n");
		return;
	}

	flash_write_protected(dev, false);

	flash_erase(dev, FLASH_TEST_REGION_OFFSET, FLASH_SECTOR_SIZE);

	flash_write_protected(dev, false);

	buf[0] = TEST_DATA_BYTE_0;
	buf[1] = TEST_DATA_BYTE_1;
	if (flash_write(dev, FLASH_TEST_REGION_OFFSET,
	    TEST_DATA_LEN, buf) != DEV_OK) {
		PRINT("SPI flash did not work as expected!\n");
		return;
	}
	PRINT("data sent %x %x\n", buf[0], buf[1]);

	if (flash_read(dev, FLASH_TEST_REGION_OFFSET,
	    TEST_DATA_LEN, buf) != DEV_OK) {
		PRINT("SPI flash did not work as expected!\n");
		return;
	}
	PRINT("data received %x %x\n", buf[0], buf[1]);

	if ((buf[0] == TEST_DATA_BYTE_0) && (buf[1] == TEST_DATA_BYTE_1)) {
		PRINT("data received matches with data sent. Good!!\n");
	} else {
		PRINT("data received does not match with data sent!!\n");
	}
}
