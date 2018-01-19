/*
 * Copyright (c) 2016 Linaro Limited
 *               2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <flash.h>
#include <device.h>
#include <stdio.h>

/* Offset between pages */
#define FLASH_TEST_OFFSET 0x40000
#define FLASH_PAGE_SIZE   4096
#define TEST_DATA_WORD_0  0x1122
#define TEST_DATA_WORD_1  0xaabb
#define TEST_DATA_WORD_2  0xabcd
#define TEST_DATA_WORD_3  0x1234

#define FLASH_TEST_OFFSET2 0x41234
#define FLASH_TEST_PAGE_IDX 37

void main(void)
{
	struct device *flash_dev;
	u32_t buf_array_1[4] = { TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3 };
	u32_t buf_array_2[4] = { TEST_DATA_WORD_3, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_0 };
	u32_t buf_array_3[8] = { TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3,
				    TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3 };
	u32_t buf_word = 0;
	u32_t i, offset;

	printf("\nNordic nRF5 Flash Testing\n");
	printf("=========================\n");

	flash_dev = device_get_binding(FLASH_DEV_NAME);

	if (!flash_dev) {
		printf("Nordic nRF5 flash driver was not found!\n");
		return;
	}

	printf("\nTest 1: Flash erase page at 0x%x\n", FLASH_TEST_OFFSET);
	if (flash_erase(flash_dev, FLASH_TEST_OFFSET, FLASH_PAGE_SIZE) != 0) {
		printf("   Flash erase failed!\n");
	} else {
		printf("   Flash erase succeeded!\n");
	}

	printf("\nTest 2: Flash write (word array 1)\n");
	flash_write_protection_set(flash_dev, false);
	for (i = 0; i < ARRAY_SIZE(buf_array_1); i++) {
		offset = FLASH_TEST_OFFSET + (i << 2);
		printf("   Attempted to write %x at 0x%x\n", buf_array_1[i],
				offset);
		if (flash_write(flash_dev, offset, &buf_array_1[i],
					sizeof(u32_t)) != 0) {
			printf("   Flash write failed!\n");
			return;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(u32_t)) != 0) {
			printf("   Flash read failed!\n");
			return;
		}
		printf("   Data read: %x\n", buf_word);
		if (buf_array_1[i] == buf_word) {
			printf("   Data read matches data written. Good!\n");
		} else {
			printf("   Data read does not match data written!\n");
		}
	}
	flash_write_protection_set(flash_dev, true);

	offset = FLASH_TEST_OFFSET - FLASH_PAGE_SIZE * 2;
	printf("\nTest 3: Flash erase (4 pages at 0x%x)\n", offset);
	if (flash_erase(flash_dev, offset, FLASH_PAGE_SIZE * 4) != 0) {
		printf("   Flash erase failed!\n");
	} else {
		printf("   Flash erase succeeded!\n");
	}

	printf("\nTest 4: Flash write (word array 2)\n");
	flash_write_protection_set(flash_dev, false);
	for (i = 0; i < ARRAY_SIZE(buf_array_2); i++) {
		offset = FLASH_TEST_OFFSET + (i << 2);
		printf("   Attempted to write %x at 0x%x\n", buf_array_2[i],
				offset);
		if (flash_write(flash_dev, offset, &buf_array_2[i],
					sizeof(u32_t)) != 0) {
			printf("   Flash write failed!\n");
			return;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(u32_t)) != 0) {
			printf("   Flash read failed!\n");
			return;
		}
		printf("   Data read: %x\n", buf_word);
		if (buf_array_2[i] == buf_word) {
			printf("   Data read matches data written. Good!\n");
		} else {
			printf("   Data read does not match data written!\n");
		}
	}
	flash_write_protection_set(flash_dev, true);

	printf("\nTest 5: Flash erase page at 0x%x\n", FLASH_TEST_OFFSET);
	if (flash_erase(flash_dev, FLASH_TEST_OFFSET, FLASH_PAGE_SIZE) != 0) {
		printf("   Flash erase failed!\n");
	} else {
		printf("   Flash erase succeeded!\n");
	}

	printf("\nTest 6: Non-word aligned write (word array 3)\n");
	flash_write_protection_set(flash_dev, false);
	for (i = 0; i < ARRAY_SIZE(buf_array_3); i++) {
		offset = FLASH_TEST_OFFSET + (i << 2) + 1;
		printf("   Attempted to write %x at 0x%x\n", buf_array_3[i],
				offset);
		if (flash_write(flash_dev, offset, &buf_array_3[i],
					sizeof(u32_t)) != 0) {
			printf("   Flash write failed!\n");
			return;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(u32_t)) != 0) {
			printf("   Flash read failed!\n");
			return;
		}
		printf("   Data read: %x\n", buf_word);
		if (buf_array_3[i] == buf_word) {
			printf("   Data read matches data written. Good!\n");
		} else {
			printf("   Data read does not match data written!\n");
		}
	}
	flash_write_protection_set(flash_dev, true);

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_info info;
	int rc;

	rc = flash_get_page_info_by_offs(flash_dev, FLASH_TEST_OFFSET2, &info);

	printf("\nTest 7: Page layout API\n");

	if (!rc) {
		printf("   Offset  0x%08x:\n", FLASH_TEST_OFFSET2);
		printf("     belongs to the page %u of start offset 0x%08x\n",
		       info.index, info.start_offset);
		printf("     and the size of 0x%08x B.\n", info.size);
	} else {
		printf("   Error: flash_get_page_info_by_offs returns %d\n",
		       rc);
	}

	rc = flash_get_page_info_by_idx(flash_dev, FLASH_TEST_PAGE_IDX, &info);

	if (!rc) {
		printf("   Page of number %u has start offset 0x%08x\n",
		       FLASH_TEST_PAGE_IDX,
		       info.start_offset);
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
	printf("   write-block-size = %u\n",
	       flash_get_write_block_size(flash_dev));
}
