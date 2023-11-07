/*
 * Copyright (c) 2016 Linaro Limited
 *               2016 Intel Corporation
 *               2023 S&C Electric Company.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>

#define TEST_PARTITION   scratch_partition
#define IMAGE1_PARTITION image1_partition

#define TEST_PARTITION_OFFSET FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE FIXED_PARTITION_DEVICE(TEST_PARTITION)
#define TEST_PARTITION_SIZE   FIXED_PARTITION_SIZE(TEST_PARTITION)

#define IMAGE1_PARTITION_OFFSET FIXED_PARTITION_OFFSET(IMAGE1_PARTITION)
#define IMAGE1_PARTITION_DEVICE FIXED_PARTITION_DEVICE(IMAGE1_PARTITION)
#define IMAGE1_PARTITION_SIZE   FIXED_PARTITION_SIZE(IMAGE1_PARTITION)

/* The write block size to use for read/write operation */
#define FLASH_WRITE_SIZE 32

int main(void)
{

	const struct device *flash_dev = TEST_PARTITION_DEVICE;
	uint32_t buffer[8];
	uint32_t buffer_1[8];
	uint32_t i, j, offset, blocks, count = 0;

	printf("\n   STM32H7 Flash API Sample Application\n");
	printf("   ========================================\n");
	printf("   Available partitions on SoC Flash\n");
	printf("   image1_partition        @%x, size %d\n", IMAGE1_PARTITION_OFFSET,
	       IMAGE1_PARTITION_SIZE);
	printf("   scratch_partition       @%x, size %d\n", TEST_PARTITION_OFFSET,
	       TEST_PARTITION_SIZE);

	if (!device_is_ready(flash_dev)) {
		printf("   Flash device not ready\n");
		return 0;
	}

	printf("\nFlash erase page at 0x%x, size %d\n", TEST_PARTITION_OFFSET, TEST_PARTITION_SIZE);
	if (flash_erase(flash_dev, TEST_PARTITION_OFFSET, TEST_PARTITION_SIZE) != 0) {
		printf("   Flash erase failed!\n");
	} else {
		printf("   Flash erase succeeded!\n");
	}

	printf("\nDoing Flash write/read operations\n");
	offset = TEST_PARTITION_OFFSET;
	blocks = TEST_PARTITION_SIZE / FLASH_WRITE_SIZE;
	/* STM32 flash supports 32byte program size. So block size is 32 */
	for (i = 0; i < FLASH_WRITE_SIZE / 4; i++) {
		if ((i % 4) == 0) {
			buffer[i] = 0xaa0055ff;
		} else if ((i % 4) == 1) {
			buffer[i] = 0x0055ff00;
		} else if ((i % 4) == 2) {
			buffer[i] = 0x55ff00aa;
		} else if ((i % 4) == 3) {
			buffer[i] = 0xff00aa55;
		}
	}

	for (i = 0; i < blocks; i++) {
		if (flash_write(flash_dev, offset + (i * FLASH_WRITE_SIZE), &buffer[0],
				sizeof(buffer)) != 0) {
			printf("   Flash write failed!\n");
			return 0;
		}
		if (i && ((i % 1024) == 0)) {
			printf("   Wrote %d blocks of 32 bytes\n", i);
		}
	}
	printf("   Wrote %d blocks of %d bytes\n", i, FLASH_WRITE_SIZE);

	for (j = 0; j < blocks; j++) {
		if (flash_read(flash_dev, offset + (j * FLASH_WRITE_SIZE), &buffer_1[0],
			       sizeof(buffer_1)) != 0) {
			printf("   Flash read failed!\n");
			return 0;
		}
		for (i = 0; i < FLASH_WRITE_SIZE / 4; i++) {
			if (buffer[i] != buffer_1[i]) {
				count++;
			}
		}
		if (j && ((j % 1024) == 0)) {
			printf("   read and verified up to 0x%x\n",
			       offset + (j * FLASH_WRITE_SIZE));
		}
	}
	if (count) {
		printf("   %d words read patterns wrong\n", count);
	} else {
		printf("   %d blocks read and verified correctly\n", blocks);
	}
	return 0;
}
