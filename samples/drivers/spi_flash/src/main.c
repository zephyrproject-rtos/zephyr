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
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <misc/sys_log.h>

#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE        4096
#define TEST_DATA_BYTE_0         0x55
#define TEST_DATA_BYTE_1         0xaa
#define TEST_DATA_LEN            2

void main(void)
{
	struct device *flash_dev;
	uint8_t buf[TEST_DATA_LEN];

	SYS_LOG_INF("\nW25QXXDV SPI flash testing");
	SYS_LOG_INF("==========================");

	flash_dev = device_get_binding("W25QXXDV");

	if (!flash_dev) {
		SYS_LOG_ERR("SPI flash driver was not found!");
		return;
	}

	/* Write protection needs to be disabled in w25qxxdv flash before
	 * each write or erase. This is because the flash component turns
	 * on write protection automatically after completion of write and
	 * erase operations.
	 */
	SYS_LOG_INF("\nTest 1: Flash erase");
	flash_write_protection_set(flash_dev, false);
	if (flash_erase(flash_dev,
			FLASH_TEST_REGION_OFFSET,
			FLASH_SECTOR_SIZE) != 0) {
		SYS_LOG_INF("   Flash erase failed!");
	} else {
		SYS_LOG_INF("   Flash erase succeeded!");
	}

	SYS_LOG_INF("\nTest 2: Flash write");
	flash_write_protection_set(flash_dev, false);

	buf[0] = TEST_DATA_BYTE_0;
	buf[1] = TEST_DATA_BYTE_1;
	SYS_LOG_INF("   Attempted to write %x %x", buf[0], buf[1]);
	if (flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
	    TEST_DATA_LEN) != 0) {
		SYS_LOG_INF("   Flash write failed!");
		return;
	}

	if (flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
	    TEST_DATA_LEN) != 0) {
		SYS_LOG_INF("   Flash read failed!");
		return;
	}
	SYS_LOG_INF("   Data read %x %x", buf[0], buf[1]);

	if ((buf[0] == TEST_DATA_BYTE_0) && (buf[1] == TEST_DATA_BYTE_1)) {
		SYS_LOG_INF("   Data read matches with data written. Good!!");
	} else {
		SYS_LOG_INF("   Data read does not match with data "
			    "written!!");
	}
}
