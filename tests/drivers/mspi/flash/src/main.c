/*
 * Copyright (c) 2024 Ambiq Micro Inc. <www.ambiq.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi_emul.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/ztest.h>

#define MSPI_BUS_NODE                 DT_ALIAS(mspi0)

#define MSPI_FLASH_TEST_REGION_OFFSET 0x0

#define MSPI_FLASH_SECTOR_SIZE        4096

#define MSPI_FLASH_TEST_SIZE          3000

static const struct device *mspi_devices[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, DEVICE_DT_GET, (,))
};

static uint8_t expected[MSPI_FLASH_TEST_SIZE];
static uint8_t actual[MSPI_FLASH_TEST_SIZE];

static void prepare_test_pattern(uint32_t pattern_index, uint8_t *actualf, uint32_t len)
{
	uint32_t *pui32TxPtr = (uint32_t *)actualf;
	uint8_t *pui8TxPtr = (uint8_t *)actualf;

	switch (pattern_index) {
	case 0:
		/* 0x5555AAAA */
		for (uint32_t i = 0; i < len / 4; i++) {
			pui32TxPtr[i] = (0x5555AAAA);
		}
		break;
	case 1:
		/*  0xFFFF0000 */
		for (uint32_t i = 0; i < len / 4; i++) {
			pui32TxPtr[i] = (0xFFFF0000);
		}
		break;
	case 2:
		/* walking */
		for (uint32_t i = 0; i < len; i++) {
			pui8TxPtr[i] = 0x01 << (i % 8);
		}
		break;
	case 3:
		/* incremental from 1 */
		for (uint32_t i = 0; i < len; i++) {
			pui8TxPtr[i] = ((i + 1) & 0xFF);
		}
		break;
	case 4:
		/* decremental from 0xff */
		for (uint32_t i = 0; i < len; i++) {
			/* decrement starting from 0xff */
			pui8TxPtr[i] = (0xff - i) & 0xFF;
		}
		break;
	default:
		/* incremental from 1 */
		for (uint32_t i = 0; i < len; i++) {
			pui8TxPtr[i] = ((i + 1) & 0xFF);
		}
		break;
	}
}

static int test_multi_sector_rw(const struct device *flash_dev)
{
	int rc = 0;
	const struct flash_pages_layout *layout = NULL;
	size_t layout_size = 0;
	size_t min_page_size = -1;
	size_t offs;

	TC_PRINT("\n===================================================================\n");
	TC_PRINT("Perform test on multiple consequtive sectors on %s\n", flash_dev->name);

	TC_PRINT("\nTest 0: Get Flash page layout\n");

	const struct flash_driver_api *api = flash_dev->api;

	api->page_layout(flash_dev, &layout, &layout_size);

	if (layout && layout_size) {
		TC_PRINT("----pages-------size----\n");
		for (int i = 0; i < layout_size; ++i) {
			TC_PRINT("%2d: 0x%-8X  0x%-8x\n", i, layout[i].pages_count,
				 layout[i].pages_size);
			min_page_size = MIN(min_page_size, layout[i].pages_size);
		}
	} else {
		TC_PRINT("Empty flash_pages_layout!\n");
		return TC_FAIL;
	}

	TC_PRINT("\nPage size selected: %d\n", min_page_size);

	for (int i = 0; i < MSPI_FLASH_TEST_SIZE; i += min_page_size) {
		prepare_test_pattern(i % 5, expected + i,
				     MIN(min_page_size, MSPI_FLASH_TEST_SIZE - i));
	}

	TC_PRINT("\nTest 1: Flash erase\n");

	/* Full flash erase if MSPI_FLASH_TEST_REGION_OFFSET = 0 and
	 * MSPI_FLASH_SECTOR_SIZE = flash size
	 * Erase 2 sectors for check for erase of consequtive sectors
	 */
	rc = flash_erase(flash_dev, MSPI_FLASH_TEST_REGION_OFFSET, MSPI_FLASH_SECTOR_SIZE * 2);
	if (rc != 0) {
		TC_PRINT("Flash erase failed! %d\n", rc);
		return TC_FAIL;
	}
	/* Read the content and check for erased */
	memset(actual, 0, MSPI_FLASH_TEST_SIZE);

	offs = MSPI_FLASH_TEST_REGION_OFFSET;
	while (offs < MSPI_FLASH_TEST_REGION_OFFSET + 2 * MSPI_FLASH_SECTOR_SIZE) {
		rc = flash_read(flash_dev, offs, actual, MSPI_FLASH_TEST_SIZE);
		if (rc != 0) {
			TC_PRINT("Flash read failed! %d\n", rc);
			return TC_FAIL;
		}
		if (actual[0] != 0xff) {
			TC_PRINT("Flash erase failed at offset 0x%x got 0x%x\n", offs,
					actual[0]);
			return TC_FAIL;
		}
		offs += MSPI_FLASH_SECTOR_SIZE;
	}
	TC_PRINT("Flash erase succeeded!\n");

	TC_PRINT("\nTest 2: Flash write\n");

	offs = MSPI_FLASH_TEST_REGION_OFFSET;
	while (offs < MSPI_FLASH_TEST_REGION_OFFSET + 2 * MSPI_FLASH_SECTOR_SIZE) {
		TC_PRINT("\nAttempting to write %zu bytes at offset 0x%x\n", MSPI_FLASH_TEST_SIZE,
			 offs);
		rc = flash_write(flash_dev, offs, expected, MSPI_FLASH_TEST_SIZE);
		if (rc != 0) {
			TC_PRINT("Flash write failed! %d\n", rc);
			return TC_FAIL;
		}

		memset(actual, 0, MSPI_FLASH_TEST_SIZE);
		rc = flash_read(flash_dev, offs, actual, MSPI_FLASH_TEST_SIZE);
		if (rc != 0) {
			TC_PRINT("Flash read failed! %d\n", rc);
			return TC_FAIL;
		}

		if (memcmp(expected, actual, MSPI_FLASH_TEST_SIZE) == 0) {
			TC_PRINT("Data read matches data written. Good!!\n");
		} else {
			const uint8_t *wp = expected;
			const uint8_t *rp = actual;
			const uint8_t *rpe = rp + MSPI_FLASH_TEST_SIZE;
			int count = 0;

			TC_PRINT("Data read does not match data written!!\n");
			while (rp < rpe) {
				if (*rp != *wp) {
					TC_PRINT("%08x wrote %02x read %02x MISMATCH\n",
						 (uint32_t)(offs + (rp - actual)), *wp, *rp);
					count++;
				}
				if (count > 100) {
					TC_PRINT("Too many data mismatch!!\n");
					break;
				}
				++rp;
				++wp;
			}
			return TC_FAIL;
		}
		offs += MSPI_FLASH_SECTOR_SIZE;
	}

	return TC_PASS;
}

ZTEST(mspi_flash, test_multi_sector_rw)
{

	for (int idx = 0; idx < ARRAY_SIZE(mspi_devices); ++idx) {

		zassert_true(device_is_ready(mspi_devices[idx]),
					     "flash%d is not ready", idx);
		zassert_true(test_multi_sector_rw(mspi_devices[idx]) == TC_PASS);

	}

}

ZTEST_SUITE(mspi_flash, NULL, NULL, NULL, NULL, NULL);
