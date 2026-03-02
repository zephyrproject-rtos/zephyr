/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/dfu_boot.h>

#define SLOT0_PARTITION		slot0_partition
#define SLOT1_PARTITION		slot1_partition

#define SLOT0_PARTITION_ID	PARTITION_ID(SLOT0_PARTITION)
#define SLOT1_PARTITION_ID	PARTITION_ID(SLOT1_PARTITION)

ZTEST(dfu_boot_api, test_get_flash_area_id)
{
	int area_id;

	/* Test valid slots */
	area_id = dfu_boot_get_flash_area_id(0);
	zassert_equal(area_id, SLOT0_PARTITION_ID, "Slot 0 should return slot0_partition ID");

	area_id = dfu_boot_get_flash_area_id(1);
	zassert_equal(area_id, SLOT1_PARTITION_ID, "Slot 1 should return slot1_partition ID");

	/* Test invalid slot */
	area_id = dfu_boot_get_flash_area_id(-1);
	zassert_true(area_id < 0, "Invalid slot should return negative value");
}

ZTEST(dfu_boot_api, test_get_erased_val)
{
	uint8_t erased_val;
	int rc;

	/* Test slot 0 */
	rc = dfu_boot_get_erased_val(0, &erased_val);
	zassert_equal(rc, 0, "Getting erased value for slot 0 should succeed");
	/* Most flash devices have 0xFF as erased value */
	zassert_true(erased_val == 0xFF || erased_val == 0x00,
		     "Erased value should be 0xFF or 0x00");

	/* Test slot 1 */
	rc = dfu_boot_get_erased_val(1, &erased_val);
	zassert_equal(rc, 0, "Getting erased value for slot 1 should succeed");

	/* Test invalid slot */
	rc = dfu_boot_get_erased_val(-1, &erased_val);
	zassert_true(rc < 0, "Invalid slot should return error");
}

ZTEST(dfu_boot_api, test_read)
{
	uint8_t buf[32];
	int rc;

	/* Test reading from slot 1 */
	rc = dfu_boot_read(1, 0, buf, sizeof(buf));
	zassert_equal(rc, 0, "Reading from slot 1 should succeed");

	/* Test invalid slot */
	rc = dfu_boot_read(-1, 0, buf, sizeof(buf));
	zassert_true(rc < 0, "Reading from invalid slot should fail");
}

ZTEST(dfu_boot_api, test_flash_area_check_empty)
{
	int rc;

	/* First erase the slot to ensure it's empty */
	rc = dfu_boot_erase_slot(1);
	zassert_equal(rc, 0, "Erasing slot 1 should succeed");

	/* Check that the erased slot is detected as empty */
	rc = dfu_boot_flash_area_check_empty(SLOT1_PARTITION_ID);
	zassert_equal(rc, 1, "Erased slot should be detected as empty (return 1)");

	/* Test with invalid area_id */
	rc = dfu_boot_flash_area_check_empty(-1);
	zassert_true(rc < 0, "Invalid area_id should return error");

	/* Write some data to make the slot non-empty */
	const struct flash_area *fa;

	rc = flash_area_open(SLOT1_PARTITION_ID, &fa);
	zassert_equal(rc, 0, "Flash area open should succeed");

	uint8_t test_data[4] = {0x12, 0x34, 0x56, 0x78};

	rc = flash_area_write(fa, 0, test_data, sizeof(test_data));
	zassert_equal(rc, 0, "Flash write should succeed");

	flash_area_close(fa);

	/* Check that the non-empty slot is detected as not empty */
	rc = dfu_boot_flash_area_check_empty(SLOT1_PARTITION_ID);
	zassert_equal(rc, 0, "Non-empty slot should be detected as not empty (return 0)");

	/* Clean up: erase the slot again */
	rc = dfu_boot_erase_slot(1);
	zassert_equal(rc, 0, "Erasing slot 1 should succeed");
}

ZTEST(dfu_boot_api, test_erase_slot)
{
	const struct flash_area *fa;
	uint32_t temp;
	off_t offs;
	int rc;

	rc = flash_area_open(SLOT1_PARTITION_ID, &fa);
	zassert_equal(rc, 0, "Flash area open should succeed");

	/* Erase the slot using the new API */
	rc = dfu_boot_erase_slot(1);
	zassert_equal(rc, 0, "Erasing slot 1 should succeed");

	/* Verify the slot is erased */
	for (offs = 0; offs < fa->fa_size && offs < 1024; offs += sizeof(temp)) {
		rc = flash_area_read(fa, offs, &temp, sizeof(temp));
		zassert_equal(rc, 0, "Reading from flash should succeed");
		zassert_equal(temp, 0xFFFFFFFF, "Flash should be erased (0xFF)");
	}

	flash_area_close(fa);
}

ZTEST(dfu_boot_api, test_get_swap_type)
{
	int swap_type;

	/* Test getting swap type for image 0 */
	swap_type = dfu_boot_get_swap_type(0);
	zassert_true(swap_type >= DFU_BOOT_SWAP_TYPE_NONE &&
		     swap_type <= DFU_BOOT_SWAP_TYPE_UNKNOWN,
		     "Swap type should be a valid value");
}

ZTEST(dfu_boot_api, test_get_image_start_offset)
{
	size_t offset;

	/* Test getting image start offset for slot 0 */
	offset = dfu_boot_get_image_start_offset(0);
	/* Offset can be 0 or a valid positive value depending on configuration */
	zassert_true(offset >= 0, "Image start offset should be non-negative");

	/* Test getting image start offset for slot 1 */
	offset = dfu_boot_get_image_start_offset(1);
	zassert_true(offset >= 0, "Image start offset should be non-negative");
}

ZTEST(dfu_boot_api, test_get_trailer_status_offset)
{
	const struct flash_area *fa;
	size_t offset;
	int rc;

	rc = flash_area_open(SLOT1_PARTITION_ID, &fa);
	zassert_equal(rc, 0, "Flash area open should succeed");

	offset = dfu_boot_get_trailer_status_offset(fa->fa_size);
	zassert_true(offset < fa->fa_size, "Trailer status offset should be within area size");

	flash_area_close(fa);
}

ZTEST(dfu_boot_api, test_read_img_info_no_image)
{
	struct dfu_boot_img_info info;
	int rc;

	/* First erase the slot to ensure no valid image */
	rc = dfu_boot_erase_slot(1);
	zassert_equal(rc, 0, "Erasing slot 1 should succeed");

	/* Try to read image info from erased slot */
	rc = dfu_boot_read_img_info(1, &info);
	zassert_true(rc != 0, "Reading image info from erased slot should fail");
}

ZTEST(dfu_boot_api, test_validate_header_invalid)
{
	uint8_t invalid_header[64];
	struct dfu_boot_img_info info;
	int rc;

	/* Fill with invalid data */
	memset(invalid_header, 0x00, sizeof(invalid_header));

	/* Test with invalid header */
	rc = dfu_boot_validate_header(invalid_header, sizeof(invalid_header), &info);
	zassert_true(rc != 0, "Validating invalid header should fail");

	/* Test with NULL data */
	rc = dfu_boot_validate_header(NULL, sizeof(invalid_header), &info);
	zassert_true(rc != 0, "Validating NULL data should fail");

	/* Test with insufficient length */
	rc = dfu_boot_validate_header(invalid_header, 4, &info);
	zassert_true(rc != 0, "Validating with insufficient length should fail");
}

ZTEST_SUITE(dfu_boot_api, NULL, NULL, NULL, NULL, NULL);
