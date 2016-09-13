/*
 * Copyright (c) 2016 Linaro Limited
 *               2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

void main(void)
{
	struct device *flash_dev;
	uint32_t buf_array_1[4] = { TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3 };
	uint32_t buf_array_2[4] = { TEST_DATA_WORD_3, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_0 };
	uint32_t buf_array_3[8] = { TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3,
				    TEST_DATA_WORD_0, TEST_DATA_WORD_1,
				    TEST_DATA_WORD_2, TEST_DATA_WORD_3 };
	uint32_t buf_word = 0;
	uint32_t i, offset;

	printf("\nNordic nRF5 Flash Testing\n");
	printf("=========================\n");

	flash_dev = device_get_binding("NRF5_FLASH");

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
					sizeof(uint32_t)) != 0) {
			printf("   Flash write failed!\n");
			return;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(uint32_t)) != 0) {
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
					sizeof(uint32_t)) != 0) {
			printf("   Flash write failed!\n");
			return;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(uint32_t)) != 0) {
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
					sizeof(uint32_t)) != 0) {
			printf("   Flash write failed!\n");
			return;
		}
		printf("   Attempted to read 0x%x\n", offset);
		if (flash_read(flash_dev, offset, &buf_word,
					sizeof(uint32_t)) != 0) {
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
}
