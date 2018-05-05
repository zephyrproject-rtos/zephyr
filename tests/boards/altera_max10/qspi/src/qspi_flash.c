/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <flash.h>
#include <device.h>
#include <ztest.h>

#define MAX_NUM_OF_SECTORS		1024
#define NUM_OF_SECTORS_TO_TEST		4
#define FLASH_SECTOR_SIZE		65536
#define TEST_DATA_LEN			4

void test_qspi_flash(void)
{
	struct device *flash_dev;
	u32_t i, offset, rd_val, wr_val;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NIOS2_QSPI_DEV_NAME);
	zassert_equal(!flash_dev, TC_PASS, "Flash device not found!");

	for (i = 0; i < NUM_OF_SECTORS_TO_TEST; i++) {
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


		/* Flash Lock Test */
		TC_PRINT("	Flash Lock Test...");
		zassert_equal(flash_write_protection_set(flash_dev, true),
				TC_PASS, "Flash write protection call failed!");
		/* Ignore erase failure as it is expected */
		flash_erase(flash_dev, offset, FLASH_SECTOR_SIZE);
		zassert_equal(flash_read(flash_dev, offset,
				&rd_val, TEST_DATA_LEN),
				TC_PASS, "Flash read call failed!");
		/*
		 * we should read back the previous value (wr_val)
		 * as we have locked the flash which will block erase
		 * and write operations.
		 */
		zassert_equal(rd_val != wr_val, TC_PASS,
					"Flash Lock Test failed!!");
		TC_PRINT("PASS\n");


		/* Flash Unlock Test */
		TC_PRINT("	Flash Unlock Test...");
		zassert_equal(flash_write_protection_set(flash_dev, false),
				TC_PASS, "Flash write protection call failed!");
		zassert_equal(flash_erase(flash_dev,
				offset, FLASH_SECTOR_SIZE),
				TC_PASS, "Flash erase call failed!");
		zassert_equal(flash_read(flash_dev, offset,
				&rd_val, TEST_DATA_LEN),
				TC_PASS, "Flash read call failed!");
		/* In case of erase all bits will be set to 1 */
		wr_val = 0xffffffff;
		zassert_equal(rd_val != wr_val, TC_PASS,
					"Flash Unlock Test failed!!");
		TC_PRINT("PASS\n");
	}
}

void test_main(void)
{
	ztest_test_suite(nios2_qspi_test_suite,
			ztest_unit_test(test_qspi_flash));
	ztest_run_test_suite(nios2_qspi_test_suite);
}
