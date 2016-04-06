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
	struct device *flash_dev;
	uint8_t buf[TEST_DATA_LEN];

	PRINT("SPI flash testing!\n");

	flash_dev = device_get_binding("W25QXXDV");

	if (!flash_dev) {
		PRINT("SPI flash driver was not found!\n");
		return;
	}

	flash_write_protection_set(flash_dev, false);

	flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET, FLASH_SECTOR_SIZE);

	flash_write_protection_set(flash_dev, false);

	buf[0] = TEST_DATA_BYTE_0;
	buf[1] = TEST_DATA_BYTE_1;
	if (flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
	    TEST_DATA_LEN) != 0) {
		PRINT("flash write failed!\n");
		return;
	}
	PRINT("data written %x %x\n", buf[0], buf[1]);

	if (flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
	    TEST_DATA_LEN) != 0) {
		PRINT("flash read failed!\n");
		return;
	}
	PRINT("data read %x %x\n", buf[0], buf[1]);

	if ((buf[0] == TEST_DATA_BYTE_0) && (buf[1] == TEST_DATA_BYTE_1)) {
		PRINT("data read matches with data written. Good!!\n");
	} else {
		PRINT("data read does not match with data written!!\n");
	}
}
