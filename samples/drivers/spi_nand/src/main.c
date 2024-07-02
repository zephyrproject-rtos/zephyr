/*
 * Copyright (c) 2022 Macronix International Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if DT_NODE_HAS_STATUS(DT_INST(0, jedec_spi_nand), okay)
#define FLASH_NAME "JEDEC SPI-NAND"
#else
#error Unsupported flash driver
#endif

#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_BLOCK_SIZE         0x20000U

void main(void)
{
	static uint8_t expected[2048] = { 0 };
	const size_t len = sizeof(expected);
	static uint8_t buf[sizeof(expected)];
	int rc;

	printf("\n" FLASH_NAME " SPI NAND Flash testing\n");
	printf("==========================\n");

	const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, jedec_spi_nand));


	if (!flash_dev) {
		printf("SPI NAND Flash driver %s was not found!\n",
		       flash_dev->name);
		return;
	}

	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	printf("\nTest 1: Flash erase\n");

	rc = flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET,
			 FLASH_BLOCK_SIZE);
	if (rc != 0) {
		printf("Flash erase failed! %d\n", rc);
	} else {
		printf("Flash erase succeeded!\n");
	}

	printf("\nTest 2: Flash write\n");

	for (int i = 0; i < sizeof(expected); i++) {
		expected[i] = (i % 0xFF);
	}
	printf("Attempting to write %u bytes\n", len);
	rc = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, expected, len);
	if (rc != 0) {
		printf("Flash write failed! %d\n", rc);
		return;
	}

	printf("\nTest 3: Flash read\n");
	memset(buf, 0, len);
	rc = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf, len);
	if (rc != 0) {
		printf("Flash read failed! %d\n", rc);
		return;
	}

	if (memcmp(expected, buf, len) == 0) {
		printf("Data read matches data written. Good!!\n");
	} else {
		const uint8_t *wp = expected;
		const uint8_t *rp = buf;
		const uint8_t *rpe = rp + len;

		printf("Data read does not match data written!!\n");
		while (rp < rpe) {
			printf("%08x wrote %02x read %02x %s\n",
			       (uint32_t)(FLASH_TEST_REGION_OFFSET + (rp - buf)),
			       *wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
			++rp;
			++wp;
		}
	}
}
