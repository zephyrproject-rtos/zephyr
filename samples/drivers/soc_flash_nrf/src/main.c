/*
 * Copyright (c) 2016 Linaro Limited
 *               2016 Intel Corporation.
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>

#define TEST_PARTITION storage_partition
#define TEST_PARTITION_OFFSET FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE FIXED_PARTITION_DEVICE(TEST_PARTITION)

#if defined(CONFIG_SOC_NRF54H20)
#define FLASH_PAGE_SIZE 2048
#else
#define FLASH_PAGE_SIZE 4096
#endif

#define TEST_DATA_SIZE_IN_BYTES 128
#define FLASH_TEST_OFFSET2      0x41234
#define FLASH_TEST_PAGE_IDX     37

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) &&	\
	defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#define FLASH_PE_RUNTIME_CHECK(cond) (cond)
#elif defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
#define FLASH_PE_RUNTIME_CHECK(cond) (true)
#else
/* Assumed IS_ENABLED(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE) */
#define FLASH_PE_RUNTIME_CHECK(cond) (false)
#endif

/**
 * Depending on value of condition erase a device or not. The condition
 * is supposed to be value of erase_requirement picked up from
 * flash_parameters.flags for device.
 */
static void erase_when_needed(const struct device *dev, bool condition,
			     uint32_t off, uint32_t size)
{
	/*
	 * Alwayes invoke erase when there are only erase requiring devices,
	 * never invoke erase when there are no devices requiring erase,
	 * always check condition if there are both kind of devices.
	 */
	if (FLASH_PE_RUNTIME_CHECK(condition)) {
		if (flash_erase(dev, off, size) != 0) {
			printf("   Erase failed!\n");
		} else {
			printf("   Erase succeeded!\n");
		}
	} else {
		(void)condition;
		printf("   Erase not required by device\n");
	}
}

/*
 * Fill test data with incrementing values
 */
static void prepare_test_data(uint8_t *test_data_buf)
{
	uint32_t counter;

	for (counter = 0; counter < TEST_DATA_SIZE_IN_BYTES; counter++) {
		*(test_data_buf + counter) = counter;
	}
}

/*
 * The function align writes with write-block-size of a device,
 * the additional_address_offset parameter can be
 * used to de-align writes by a provided value.
 */
static void write_and_verify_test_data(const struct device *flash_dev, uint8_t *test_data,
				       uint8_t write_block_size, uint8_t addtitonal_address_offset)
{
	uint32_t i, offset;
	uint8_t write_cycles;
	uint8_t read_buffer[TEST_DATA_SIZE_IN_BYTES];

	write_cycles = TEST_DATA_SIZE_IN_BYTES / write_block_size;
	printf("	Write block size: %u\n", write_block_size);
	printf("	Required write cycles for given data and memory: %u\n", write_cycles);

	for (i = 0U; i < write_cycles; i++) {
		offset = TEST_PARTITION_OFFSET + i * write_block_size + addtitonal_address_offset;
		printf("	Writing %u data bytes to 0x%x\n", write_block_size, offset);
		if (flash_write(flash_dev, offset, &test_data[i * write_block_size],
				write_block_size) != 0) {
			printf("	Write failed!\n");
			return;
		}
		printf("	Reading %u data bytes from 0x%x\n", write_block_size, offset);
		if (flash_read(flash_dev, offset, &read_buffer[i * write_block_size],
			       write_block_size) != 0) {
			printf("	Read failed!\n");
			return;
		}
	}

	if (memcmp(test_data, read_buffer, TEST_DATA_SIZE_IN_BYTES)) {
		printf("	Data read does not match data written!\n");
	} else {
		printf("	Data read matches data written. Good!\n");
	}
}

int main(void)
{
	uint32_t offset;
	const struct device *flash_dev = TEST_PARTITION_DEVICE;
	struct flash_parameters flash_params;

	uint8_t test_data[TEST_DATA_SIZE_IN_BYTES];

	memcpy(&flash_params, flash_get_parameters(flash_dev), sizeof(flash_params));
	prepare_test_data(test_data);

	printf("\nNordic nRF5 Internal Storage Sample\n");
	printf("=====================================\n");

	if (!device_is_ready(flash_dev)) {
		printf("Internal storage device not ready\n");
		return 0;
	}

	printf("\nTest 1: Internal storage erase page at 0x%x\n", TEST_PARTITION_OFFSET);
	erase_when_needed(flash_dev,
			  flash_params_get_erase_cap(&flash_params) & FLASH_ERASE_C_EXPLICIT,
			  TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE);

	printf("\nTest 2: Internal storage write\n");
	write_and_verify_test_data(flash_dev, test_data, flash_params.write_block_size, 0);

	offset = TEST_PARTITION_OFFSET;
	printf("\nTest 3: Internal storage erase (2 pages at 0x%x)\n", offset);
	erase_when_needed(flash_dev,
			  flash_params_get_erase_cap(&flash_params) & FLASH_ERASE_C_EXPLICIT,
			  offset, FLASH_PAGE_SIZE * 2);

	printf("\nTest 4: Internal storage erase page at 0x%x\n", TEST_PARTITION_OFFSET);
	erase_when_needed(flash_dev,
			  flash_params_get_erase_cap(&flash_params) & FLASH_ERASE_C_EXPLICIT,
			  TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE);

	printf("\nTest 5: Non-word aligned write\n");

	if (flash_params.write_block_size != 1) {
		printf("	Skipping unaligned write, not supported\n");
	} else {
		write_and_verify_test_data(flash_dev, test_data, flash_params.write_block_size, 1);
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_info info;
	int rc;

	rc = flash_get_page_info_by_offs(flash_dev, FLASH_TEST_OFFSET2, &info);

	printf("\nTest 6: Page layout API\n");

	if (!rc) {
		printf("   Offset  0x%08x:\n", FLASH_TEST_OFFSET2);
		printf("     belongs to the page %u of start offset 0x%08lx\n",
		       info.index, (unsigned long) info.start_offset);
		printf("     and the size of 0x%08x B.\n", info.size);
	} else {
		printf("   Error: flash_get_page_info_by_offs returns %d\n",
		       rc);
	}

	rc = flash_get_page_info_by_idx(flash_dev, FLASH_TEST_PAGE_IDX, &info);

	if (!rc) {
		printf("   Page of number %u has start offset 0x%08lx\n",
		       FLASH_TEST_PAGE_IDX,
		       (unsigned long) info.start_offset);
		printf("     and size of 0x%08x B.\n", info.size);
		if (info.index == FLASH_TEST_PAGE_IDX) {
			printf("     Page index resolved properly\n");
		} else {
			printf("     ERROR: Page index resolved to %u\n",
			       info.index);
		}
	} else {
		printf("   Error: flash_get_page_info_by_idx returns %d\n", rc);
	}

	printf("   SoC flash consists of %u pages.\n",
	       flash_get_page_count(flash_dev));

#endif

	printf("\nTest 7: Write block size API\n");
	printf("   write-block-size = %u\n", flash_params.write_block_size);
	printf("\nFinished!\n");
	return 0;
}
