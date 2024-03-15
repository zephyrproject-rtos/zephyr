/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_flash);

#ifdef CONFIG_BOOTLOADER_MCUBOOT

#define TEST_FLASH_PART_NODE \
	DT_NODELABEL(boot_partition)

#else

#define TEST_FLASH_PART_NODE \
	DT_NODELABEL(slot1_partition)

#endif

#define TEST_FLASH_PART_OFFSET \
	DT_REG_ADDR(TEST_FLASH_PART_NODE)

#define TEST_FLASH_PART_SIZE \
	DT_REG_SIZE(TEST_FLASH_PART_NODE)

#define TEST_FLASH_PART_END_OFFSET \
	(TEST_FLASH_PART_OFFSET + TEST_FLASH_PART_SIZE)

#define TEST_FLASH_CONTROLLER_NODE \
	DT_MTD_FROM_FIXED_PARTITION(TEST_FLASH_PART_NODE)

static const struct device *flash_controller = DEVICE_DT_GET(TEST_FLASH_CONTROLLER_NODE);
static uint8_t test_write_block[128];
static uint8_t test_read_block[128];

static void test_flash_fill_test_write_block(void)
{
	for (uint8_t i = 0; i < sizeof(test_write_block); i++) {
		test_write_block[i] = i;
	}
}

static void *test_flash_setup(void)
{
	test_flash_fill_test_write_block();
	return NULL;
}

static bool test_flash_mem_is_set_to(const uint8_t *data, uint8_t to, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (data[i] != to) {
			return false;
		}
	}

	return true;
}

static bool test_flash_is_erased(off_t offset, size_t size)
{
	static uint8_t test_erase_buffer[99];
	const struct flash_parameters *parameters = flash_get_parameters(flash_controller);
	size_t remaining;
	size_t readsize;

	while (offset < size) {
		remaining = size - (size_t)offset;
		readsize = MIN(sizeof(test_erase_buffer), remaining);

		if (flash_read(flash_controller, offset, test_erase_buffer, readsize) < 0) {
			return false;
		}

		if (!test_flash_mem_is_set_to(test_erase_buffer, parameters->erase_value,
					      readsize)) {
			return false;
		}

		offset += (off_t)readsize;
	}

	return true;
}

static bool test_flash_verify_partition_is_erased(void)
{
	return test_flash_is_erased(TEST_FLASH_PART_OFFSET, TEST_FLASH_PART_SIZE);
}

static int test_flash_erase_partition(void)
{
	LOG_INF("Erasing section of size %zu at offset %zu controlled by %s",
		TEST_FLASH_PART_SIZE,
		TEST_FLASH_PART_OFFSET,
		flash_controller->name);

	return flash_erase(flash_controller, TEST_FLASH_PART_OFFSET,
			   TEST_FLASH_PART_SIZE);
}

static void test_flash_before(void *f)
{
	ARG_UNUSED(f);
	__ASSERT(test_flash_erase_partition() == 0,
		 "Failed to erase partition");
	__ASSERT(test_flash_verify_partition_is_erased(),
		 "Failed to erase partition");
}

static void test_flash_write_block_at_offset(off_t offset, size_t size)
{
	zassert_ok(flash_write(flash_controller, offset, test_write_block, size),
		   "Failed to write block at offset %zu, of size %zu", (size_t)offset, size);
	zassert_ok(flash_read(flash_controller, offset, test_read_block, size),
		   "Failed to read block at offset %zu, of size %zu", (size_t)offset, size);
	zassert_ok(memcmp(test_write_block, test_read_block, size),
		   "Failed to write block at offset %zu, of size %zu to page", (size_t)offset,
		   size);
}

static void test_flash_write_across_page_boundary(const struct flash_pages_info *info,
						  size_t write_block_size)
{
	off_t page_boundary = info->start_offset;
	uint32_t page0_index = info->index - 1;
	uint32_t page1_index = info->index;
	off_t cross_write_start_offset = page_boundary - (off_t)write_block_size;
	size_t cross_write_size = write_block_size * 2;

	LOG_INF("Writing across page boundary at %zu, between page index %u and %u",
		(size_t)page_boundary,
		page0_index,
		page1_index);

	test_flash_write_block_at_offset(cross_write_start_offset, cross_write_size);
}

static bool test_flash_write_across_page_boundaries(const struct flash_pages_info *info,
						    void *data)
{
	size_t write_block_size = *((size_t *)data);

	if (info->start_offset <= TEST_FLASH_PART_OFFSET) {
		/* Not yet reached second page within partition */
		return true;
	}

	if (info->start_offset >= TEST_FLASH_PART_END_OFFSET) {
		/* Reached first page after partition end */
		return false;
	}

	test_flash_write_across_page_boundary(info, write_block_size);
	return true;
}

ZTEST(flash_page_layout, test_write_across_page_boundaries_in_part)
{
	const struct flash_parameters *parameters = flash_get_parameters(flash_controller);
	size_t write_block_size = parameters->write_block_size;

	flash_page_foreach(flash_controller, test_flash_write_across_page_boundaries,
			   &write_block_size);
}

static void test_flash_erase_page(const struct flash_pages_info *info)
{
	off_t page_offset = info->start_offset;
	size_t page_size = info->size;
	size_t page_index = info->index;

	LOG_INF("Erasing page at %zu of size %zu with index %zu",
		(size_t)page_offset, page_size, page_index);

	zassert_ok(flash_erase(flash_controller, page_offset, page_size),
		   "Failed to erase page");

	zassert_true(test_flash_is_erased(page_offset, page_size),
		     "Failed to erase page");
}

static bool test_flash_erase_pages(const struct flash_pages_info *info, void *data)
{
	if (info->start_offset < TEST_FLASH_PART_OFFSET) {
		/* Not yet reached first page within partition */
		return true;
	}

	if (info->start_offset >= TEST_FLASH_PART_END_OFFSET) {
		/* Reached first page after partition end */
		return false;
	}

	test_flash_erase_page(info);
	return true;
}

ZTEST(flash_page_layout, test_erase_single_pages_in_part)
{
	const struct flash_parameters *parameters = flash_get_parameters(flash_controller);
	size_t write_block_size = parameters->write_block_size;

	flash_page_foreach(flash_controller, test_flash_write_across_page_boundaries,
			   &write_block_size);

	flash_page_foreach(flash_controller, test_flash_erase_pages, NULL);
}

ZTEST_SUITE(flash_page_layout, NULL, test_flash_setup, test_flash_before, NULL, NULL);
