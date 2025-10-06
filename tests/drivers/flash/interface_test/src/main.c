/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>

#ifdef CONFIG_BOOTLOADER_MCUBOOT
#define TEST_FLASH_PART_NODE DT_NODELABEL(boot_partition)
#else
#define TEST_FLASH_PART_NODE DT_NODELABEL(slot1_partition)
#endif

#define TEST_FLASH_PART_OFFSET DT_REG_ADDR(TEST_FLASH_PART_NODE)
#define TEST_FLASH_PART_SIZE   DT_REG_SIZE(TEST_FLASH_PART_NODE)

#define TEST_FLASH_CONTROLLER_NODE DT_MTD_FROM_FIXED_PARTITION(TEST_FLASH_PART_NODE)

#define PATTERN_SIZE 256

static const struct device *flash_controller = DEVICE_DT_GET(TEST_FLASH_CONTROLLER_NODE);

static uint8_t pattern[PATTERN_SIZE];

static const off_t test_area_offset = TEST_FLASH_PART_OFFSET;
static const size_t test_area_size = TEST_FLASH_PART_SIZE & ~(PATTERN_SIZE - 1);
static const off_t test_area_end = test_area_offset + test_area_size;
static const size_t number_of_slots = test_area_size / PATTERN_SIZE;

static int verify_block(off_t pos, uint8_t *expected_data, size_t size)
{
	uint8_t buffer[size];

	if (flash_read(flash_controller, pos, buffer, size) != 0) {
		return -EIO;
	}

	if (memcmp(expected_data, buffer, size) != 0) {
		return -EBADMSG;
	}

	return 0;
}

static void *flash_setup(void)
{
	bool pattern_available = true;

	TC_PRINT("test_area_size = %zu MB\n", test_area_size >> 20);

	/* Initialize pattern */
	for (int i = 0; i < sizeof(pattern); i++) {
		pattern[i] = i;
	}

	for (off_t pos = test_area_offset; pos < test_area_end; pos += PATTERN_SIZE) {
		int res = verify_block(pos, pattern, sizeof(pattern));

		if (res == -EBADMSG) {
			pattern_available = false;
			break;
		} else if (res < 0) {
			/* fail at any other errors */
			zassert_ok(res);
			return NULL;
		}
	}

	if (!pattern_available) {
		TC_PRINT("Erasing test area\n");
		zassert_ok(flash_erase(flash_controller, TEST_FLASH_PART_OFFSET, test_area_size));

		TC_PRINT("Writing pattern\n");
		for (off_t pos = test_area_offset; pos < test_area_end; pos += PATTERN_SIZE) {
			zassert_ok(flash_write(flash_controller, pos, pattern, sizeof(pattern)));
		}
	} else {
		TC_PRINT("Pattern is already available\n");
	}

	return NULL;
}

ZTEST(flash_interface, test_sequential_read_pattern)
{
	int amount = CONFIG_TEST_SEQUENTIAL_READ_PATTERN_AMOUNT;
	off_t slot = 0;

	zassert_not_equal(number_of_slots, 0);

	for (int i = 0; i < amount; i++) {
		if ((i & 255) == 0) {
			TC_PRINT("Verifying pattern sequentially (%i/%i)\n", i + 1, amount);
		}
		zassert_ok(verify_block(test_area_offset + (slot * PATTERN_SIZE), pattern,
					sizeof(pattern)));
		slot = (slot + 1) % number_of_slots;
	}
}

ZTEST(flash_interface, test_sequential_alternating_read_pattern)
{
	int amount = CONFIG_TEST_SEQUENTIAL_ALTERNATING_READ_PATTERN_AMOUNT;
	off_t slot1 = 0;
	off_t slot2 = number_of_slots / 2;

	zassert_not_equal(number_of_slots, 0);

	for (int i = 0; i < amount; i++) {
		if ((i & 255) == 0) {
			TC_PRINT(
				"Verifying pattern sequentially on alternating positions (%i/%i)\n",
				i + 1, amount);
		}
		zassert_ok(verify_block(test_area_offset + (slot1 * PATTERN_SIZE), pattern,
					sizeof(pattern)));
		zassert_ok(verify_block(test_area_offset + (slot2 * PATTERN_SIZE), pattern,
					sizeof(pattern)));
		slot1 = (slot1 + 1) % number_of_slots;
		slot2 = (slot2 + 1) % number_of_slots;
	}
}

ZTEST_SUITE(flash_interface, NULL, flash_setup, NULL, NULL, NULL);
