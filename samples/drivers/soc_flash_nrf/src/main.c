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


#define TEST_PARTITION	storage_partition

#define TEST_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(TEST_PARTITION)

#define FLASH_PAGE_SIZE   4096
#define TEST_DATA_WORD_0  0x1122
#define TEST_DATA_WORD_1  0xaabb
#define TEST_DATA_WORD_2  0xabcd
#define TEST_DATA_WORD_3  0x1234

#define FLASH_TEST_OFFSET2 0x41234
#define FLASH_TEST_PAGE_IDX 37

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

int main(void)
{
	const struct device *flash_dev = TEST_PARTITION_DEVICE;
	struct flash_parameters flash_params;
	uint32_t buf_array_1[4] = { TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3 };
	uint32_t buf_array_2[4] = { TEST_DATA_WORD_3, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_0 };
	uint32_t buf_array_3[8] = { TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3,
				    TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3 };
	uint32_t buf_word = 0U;
	uint32_t i, offset;

	memcpy(&flash_params, flash_get_parameters(flash_dev), sizeof(flash_params));

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

	printf("\nTest 2: Internal storage write (word array 1)\n");
	for (i = 0U; i < ARRAY_SIZE(buf_array_1); i++) {
		offset = TEST_PARTITION_OFFSET + (i << 2);
		printf("   Attempted to write %x at 0x%x\n", buf_array_1[i],
				offset);
		if (flash_write(flash_dev, offset, &buf_array_1[i],
					sizeof(uint32_t)) != 0) {
			printf("   Write failed!\n");
			return 0;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(uint32_t)) != 0) {
			printf("   Read failed!\n");
			return 0;
		}
		printf("   Data read: %x\n", buf_word);
		if (buf_array_1[i] == buf_word) {
			printf("   Data read matches data written. Good!\n");
		} else {
			printf("   Data read does not match data written!\n");
		}
	}

	offset = TEST_PARTITION_OFFSET;
	printf("\nTest 3: Internal storage erase (2 pages at 0x%x)\n", offset);
	erase_when_needed(flash_dev,
			  flash_params_get_erase_cap(&flash_params) & FLASH_ERASE_C_EXPLICIT,
			  offset, FLASH_PAGE_SIZE * 2);
	printf("\nTest 4: Internal storage write (word array 2)\n");
	for (i = 0U; i < ARRAY_SIZE(buf_array_2); i++) {
		offset = TEST_PARTITION_OFFSET + (i << 2);
		printf("   Attempted to write %x at 0x%x\n", buf_array_2[i],
				offset);
		if (flash_write(flash_dev, offset, &buf_array_2[i],
					sizeof(uint32_t)) != 0) {
			printf("   Write failed!\n");
			return 0;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(uint32_t)) != 0) {
			printf("   Read failed!\n");
			return 0;
		}
		printf("   Data read: %x\n", buf_word);
		if (buf_array_2[i] == buf_word) {
			printf("   Data read matches data written. Good!\n");
		} else {
			printf("   Data read does not match data written!\n");
		}
	}

	printf("\nTest 5: Internal storage erase page at 0x%x\n", TEST_PARTITION_OFFSET);
	erase_when_needed(flash_dev,
			  flash_params_get_erase_cap(&flash_params) & FLASH_ERASE_C_EXPLICIT,
			  TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE);

	printf("\nTest 6: Non-word aligned write (word array 3)\n");
	for (i = 0U; i < ARRAY_SIZE(buf_array_3); i++) {
		offset = TEST_PARTITION_OFFSET + (i << 2) + 1;
		printf("   Attempted to write %x at 0x%x\n", buf_array_3[i],
				offset);
		if (flash_write(flash_dev, offset, &buf_array_3[i],
					sizeof(uint32_t)) != 0) {
			printf("   Write failed!\n");
			return 0;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(uint32_t)) != 0) {
			printf("   Read failed!\n");
			return 0;
		}
		printf("   Data read: %x\n", buf_word);
		if (buf_array_3[i] == buf_word) {
			printf("   Data read matches data written. Good!\n");
		} else {
			printf("   Data read does not match data written!\n");
		}
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_info info;
	int rc;

	rc = flash_get_page_info_by_offs(flash_dev, FLASH_TEST_OFFSET2, &info);

	printf("\nTest 7: Page layout API\n");

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

	printf("\nTest 8: Write block size API\n");
	printf("   write-block-size = %u\n", flash_params.write_block_size);

	printf("\nFinished!\n");
	return 0;
}
