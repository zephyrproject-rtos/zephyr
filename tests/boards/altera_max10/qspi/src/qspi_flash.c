/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#define MAX_NUM_OF_SECTORS		1024
#define NUM_OF_SECTORS_TO_TEST		4
#define FLASH_SECTOR_SIZE		65536
#define TEST_DATA_LEN			4

ZTEST(nios2_qspi, test_qspi_flash)
{
	const struct device *flash_dev;
	uint32_t i, offset, rd_val, wr_val;
	uint8_t wr_buf[4] = {0xAA, 0xBB, 0xCC, 0xDD};
	uint8_t rd_buf[2];

	flash_dev = DEVICE_DT_GET(DT_NODELABEL(n25q512ax3));
	zassert_true(!device_is_ready(flash_dev), TC_PASS, "Flash device is not ready!");

	for (i = 0U; i < NUM_OF_SECTORS_TO_TEST; i++) {
		TC_PRINT("\nTesting: Flash Sector-%d\n", i);
		offset = FLASH_SECTOR_SIZE * i;

		/* Flash Erase Test */
		TC_PRINT("	Flash Erase Test...");
		zassert_equal(flash_erase(flash_dev,
				offset, FLASH_SECTOR_SIZE),
				TC_PASS, "Flash erase call failed!");
		zassert_equal(flash_read(flash_dev, offset,
				&rd_val, TEST_DATA_LEN),
				TC_PASS, "Flash read call failed!");
		/* In case of erase all bits will be set to 1 */
		wr_val = 0xffffffff;
		zassert_equal(rd_val != wr_val,	TC_PASS,
					"Flash Erase Test failed!!");
		TC_PRINT("PASS\n");


		/* Flash Write & Read Test */
		TC_PRINT("	Flash Write & Read Test...");
		wr_val = 0xAABBCCDD;
		zassert_equal(flash_write(flash_dev, offset,
				&wr_val, TEST_DATA_LEN),
				TC_PASS, "Flash write call failed!");
		zassert_equal(flash_read(flash_dev, offset,
				&rd_val, TEST_DATA_LEN),
				TC_PASS, "Flash read call failed!");
		zassert_equal(rd_val != wr_val,	TC_PASS,
					"Flash Write & Read Test failed!!");
		TC_PRINT("PASS\n");


		/* Flash Unaligned Read Test */
		TC_PRINT("	Flash Unaligned Read Test...");
		zassert_equal(flash_write(flash_dev, offset + sizeof(wr_val),
				&wr_buf, sizeof(wr_buf)),
				TC_PASS, "Flash write call failed!");
		zassert_equal(flash_read(flash_dev, offset + sizeof(wr_val) + 1,
				&rd_buf, sizeof(rd_buf)),
				TC_PASS, "Flash read call failed!");
		zassert_equal(memcmp(wr_buf + 1, rd_buf, sizeof(rd_buf)),
				TC_PASS, "Flash Write & Read Test failed!!");
		TC_PRINT("PASS\n");
	}
}

ZTEST_SUITE(nios2_qspi, NULL, NULL, NULL, NULL, NULL);
