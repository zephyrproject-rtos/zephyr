/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/zsai.h>
#include <zephyr/drivers/zsai_infoword.h>
#include <zephyr/drivers/zsai_ioctl.h>
#include <stdio.h>
#include <memory.h>

#if !IS_ENABLED(CONFIG_ZSAI_SIMULATED_DEVICE_DRIVER)
#define ZSAI_DEV_NODE	DT_PARENT(DT_INST(0, soc_nv_flash))
#else
#define ZSAI_DEV_NODE	DT_NODELABEL(zsai_something)
#endif

const struct device *zsai_dev = DEVICE_DT_GET(ZSAI_DEV_NODE);

#define DATA	"I don't like this one bit. Not one bit."

int main(void)
{
	uint8_t buffer[64] = { 0 };
	size_t data_size = sizeof(DATA);
	off_t offset;
	size_t dev_size;

	printf("Starting ZSAI sample on %s\n", CONFIG_BOARD);
	printf("Device uses write block size %d\n", ZSAI_WRITE_BLOCK_SIZE(zsai_dev));

	if (ZSAI_WRITE_BLOCK_SIZE(zsai_dev) != 1) {
		off_t uncorrected = data_size;

		/* We are cutting data down to the write block aligned size */
		data_size &= ~(ZSAI_WRITE_BLOCK_SIZE(zsai_dev) - 1);

		if (data_size != uncorrected) {
			printf("Write block size %d != 1\n", ZSAI_WRITE_BLOCK_SIZE(zsai_dev));
			printf("Correcting size from %ld to %lu\n", uncorrected,
			       (unsigned long)data_size);
			printf("Yes, we are cutting data down to the required write block size\n");
		}
	}

	if (zsai_get_size(zsai_dev, &dev_size) < 0) {
		printf("Failed to get device size\n");
		return 1;
	}

	offset = dev_size - data_size;

	printf("Device has size %lu\n", (unsigned long)dev_size);

/* This one is needed because ZSAI_ERASE_VALUE will trigger build assert if no device
 * requires erase.
 */
#if IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE)
	/* Data will be written at the end of device. */
	if (ZSAI_ERASE_REQUIRED(zsai_dev)) {
		struct zsai_ioctl_range pi;

		printf("Device requires erase prior to write\n");

		/* Page info will correct offset in pi to the
		 * offset of page.
		 */
		if (zsai_get_page_info(zsai_dev, offset, &pi) < 0) {
			printf("Failed to get page info at %ld\n",
			       (long)dev_size - offset);
			return 2;
		}

		/* Erase can be done via zsai_erae or zsai_erase_range.
		 * Because we are going to erase entire page, that we
		 * have just asked information for, we will use the
		 * zsai_erase_range as it allows us to just pass the
		 * obtained info.
		 */
		printf("For offset %lu, got page with offset %lu and size %lu\n",
		       (unsigned long)offset, (unsigned long)pi.offset, (unsigned long)pi.size);
		if (zsai_erase_range(zsai_dev, &pi) < 0) {
			printf("Failed to erase page %ld to %ld\n",
			       (long)pi.offset, (long)pi.offset + (long)pi.size);
			return 3;
		}

		/* Lets check if required size is errased */
		printf("Check if offset for write is erased\n");
		if (zsai_read(zsai_dev, buffer, offset, data_size) < 0) {
			printf("Failed to read end of device\n");
			return 4;
		}

		for (int i = 0; i < data_size; i++) {
			if (buffer[i] != (uint8_t)ZSAI_ERASE_VALUE(zsai_dev)) {
				printf("Failed erase at %ld\n", (long)offset + i);
				return 5;
			}
		}
		printf("Erase was ok\n");
	}
#endif

	/* Now lets write something */
	printf("Writing DATA to device\n");
	if (zsai_write(zsai_dev, DATA, offset, data_size) < 0) {
		printf("Failed to write test data\n");
		return 6;
	}

	printf("Reading device back to buffer\n");
	if (zsai_read(zsai_dev, buffer, offset, data_size) < 0) {
		printf("Failed to read back data\n");
		return 7;
	}

	if (memcmp(buffer, DATA, data_size) != 0) {
		printf("Data read from device is not correct\n");
		return 8;
	}
	printf("Data and buffer matches\n");

	return 0;
}
