/*
 * Copyright (c) 2023 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/stm32_flash_api_extensions.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#define TEST_AREA	 storage_partition
#define TEST_AREA_OFFSET FIXED_PARTITION_OFFSET(TEST_AREA)
#define TEST_AREA_SIZE	 FIXED_PARTITION_SIZE(TEST_AREA)
#define TEST_AREA_MAX	 (TEST_AREA_OFFSET + TEST_AREA_SIZE)
#define TEST_AREA_DEVICE FIXED_PARTITION_DEVICE(TEST_AREA)

#define EXPECTED_SIZE 512

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static const struct flash_parameters *flash_params;
static uint32_t sector_mask;
static uint8_t __aligned(4) expected[EXPECTED_SIZE];

static int sector_mask_from_offset(const struct device *dev, off_t offset,
				   size_t size, uint32_t *sector_mask)
{
	struct flash_pages_info start_page, end_page;

	if (flash_get_page_info_by_offs(dev, offset, &start_page) ||
	    flash_get_page_info_by_offs(dev, offset + size - 1, &end_page)) {
		return -EINVAL;
	}

	*sector_mask = ((1UL << (end_page.index + 1)) - 1) &
		       ~((1UL << start_page.index) - 1);

	return 0;
}

static void *flash_stm32_setup(void)
{
	struct flash_stm32_ex_op_sector_wp_out wp_status;
	struct flash_stm32_ex_op_sector_wp_in wp_request;
	uint8_t buf[EXPECTED_SIZE];
	bool is_buf_clear = true;
	int rc;

	/* Check if tested region fits in flash. */
	zassert_true((TEST_AREA_OFFSET + EXPECTED_SIZE) < TEST_AREA_MAX,
		     "Test area exceeds flash size");

	zassert_true(device_is_ready(flash_dev));

	flash_params = flash_get_parameters(flash_dev);

	rc = sector_mask_from_offset(flash_dev, TEST_AREA_OFFSET, EXPECTED_SIZE,
				     &sector_mask);
	zassert_equal(rc, 0, "Cannot get sector mask");

	TC_PRINT("Sector mask for offset 0x%x size 0x%x is 0x%x\n",
	       TEST_AREA_OFFSET, EXPECTED_SIZE, sector_mask);

	/* Check if region is not write protected. */
	rc = flash_ex_op(flash_dev, FLASH_STM32_EX_OP_SECTOR_WP,
			 (uintptr_t)NULL, &wp_status);
	zassert_equal(rc, 0, "Cannot get write protect status");

	TC_PRINT("Protected sectors: 0x%x\n", wp_status.protected_mask);

	/* Remove protection if needed. */
	if (wp_status.protected_mask & sector_mask) {
		TC_PRINT("Removing write protection\n");
		wp_request.disable_mask = sector_mask;

		rc = flash_ex_op(flash_dev, FLASH_STM32_EX_OP_SECTOR_WP,
				 (uintptr_t)&wp_request, NULL);
		zassert_equal(rc, 0, "Cannot remove write protection");
	}

	/* Check if test region is not empty. */
	rc = flash_read(flash_dev, TEST_AREA_OFFSET, buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	/* Check if flash is cleared. */
	for (off_t i = 0; i < EXPECTED_SIZE; i++) {
		if (buf[i] != flash_params->erase_value) {
			is_buf_clear = false;
			break;
		}
	}

	if (!is_buf_clear) {
		TC_PRINT("Test area is not empty. Clear it before continuing.\n");
		rc = flash_erase(flash_dev, TEST_AREA_OFFSET, EXPECTED_SIZE);
		zassert_equal(rc, 0, "Flash memory not properly erased");
	}

	/* Fill test buffer with some pattern. */
	for (int i = 0; i < EXPECTED_SIZE; i++) {
		expected[i] = i;
	}

	return NULL;
}

ZTEST(flash_stm32, test_stm32_write_protection)
{
	struct flash_stm32_ex_op_sector_wp_in wp_request;
	uint8_t buf[EXPECTED_SIZE];
	int rc;

	TC_PRINT("Enabling write protection...");
	wp_request.disable_mask = 0;
	wp_request.enable_mask = sector_mask;

	rc = flash_ex_op(flash_dev, FLASH_STM32_EX_OP_SECTOR_WP,
			 (uintptr_t)&wp_request, NULL);
	zassert_equal(rc, 0, "Cannot enable write protection");
	TC_PRINT("Done\n");

	rc = flash_write(flash_dev, TEST_AREA_OFFSET, expected, EXPECTED_SIZE);
	zassert_not_equal(rc, 0, "Write suceeded");
	TC_PRINT("Write failed as expected, error %d\n", rc);

	rc = flash_read(flash_dev, TEST_AREA_OFFSET, buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	for (off_t i = 0; i < EXPECTED_SIZE; i++) {
		zassert_true(buf[i] == flash_params->erase_value,
			     "Buffer is not empty after write with protected "
			     "sectors");
	}

	TC_PRINT("Disabling write protection...");
	wp_request.disable_mask = sector_mask;
	wp_request.enable_mask = 0;

	rc = flash_ex_op(flash_dev, FLASH_STM32_EX_OP_SECTOR_WP,
			 (uintptr_t)&wp_request, NULL);
	zassert_equal(rc, 0, "Cannot disable write protection");
	TC_PRINT("Done\n");

	rc = flash_write(flash_dev, TEST_AREA_OFFSET, expected, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Write failed");
	TC_PRINT("Write suceeded\n");

	rc = flash_read(flash_dev, TEST_AREA_OFFSET, buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	zassert_equal(memcmp(buf, expected, EXPECTED_SIZE), 0,
		      "Read data doesn't match expected data");
}

ZTEST(flash_stm32, test_stm32_readout_protection_disabled)
{
	struct flash_stm32_ex_op_rdp rdp_status;
	int rc;

	rc = flash_ex_op(flash_dev, FLASH_STM32_EX_OP_RDP, (uintptr_t)NULL,
			 &rdp_status);
	zassert_equal(rc, 0, "Failed to get RDP status");
	zassert_false(rdp_status.enable, "RDP is enabled");
	zassert_false(rdp_status.permanent, "RDP is enabled permanently");

	TC_PRINT("RDP is disabled\n");
}

ZTEST_SUITE(flash_stm32, NULL, flash_stm32_setup, NULL, NULL, NULL);
