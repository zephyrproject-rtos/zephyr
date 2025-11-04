/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#define STR2(X) #X
#define STR(X) STR2(X)
#define ASCII_PATTERN(i) ((uint8_t) i % 128)

#define TEST_PARTITION storage_partition
#define TEST_PARTITION_DEVICE FIXED_PARTITION_DEVICE(TEST_PARTITION)
#define TEST_PARTITION_ADDRESS FIXED_PARTITION_ADDRESS(TEST_PARTITION)
#define TEST_PARTITION_SIZE FIXED_PARTITION_SIZE(TEST_PARTITION)

#define PAGE_SIZE 0x1000

static const struct device *flash_dev;
static uint8_t buf[PAGE_SIZE];

/* Used ROM function would print to the same console as
 * a host program, which simplify the test setup
 */
extern int ets_printf(const char *fmt, ...);

int main(void)
{
	int i;
	int ret = 0;

	flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	if (!flash_dev) {
		ets_printf("Failed to get the flash0 device.\n\r");
		return 0;
	}

	/* Workaround to wait until all is ready */
	k_sleep(K_MSEC(100));

	ets_printf("\nFlash IPM sample program (remote CPU)\n\n");
	ets_printf("This program is running on a remote CPU.\n");
	ets_printf("Partition %s will be used to perform flash API.\n\n", STR(TEST_PARTITION));
	ets_printf("To inspect the test area manually, run on HOST CPU console:\n");
	ets_printf(" $ flash read %x %x\n\n", TEST_PARTITION_ADDRESS, PAGE_SIZE);

	/* Erase flash page */
	ets_printf("Step 1: Erase\n* Erasing first %d bytes of partition.. ", PAGE_SIZE);
	if (!flash_erase(flash_dev, TEST_PARTITION_ADDRESS, PAGE_SIZE)) {
		ets_printf("OK\n");
	} else {
		ets_printf("ERR\n");
		return -1;
	}
	ets_printf("* Verifing page erased.. ");
	if (flash_read(flash_dev, TEST_PARTITION_ADDRESS, buf, PAGE_SIZE)) {
		ets_printf("ERR\n");
		return -1;
	}
	for (i = 0, ret = 0; i < PAGE_SIZE; i++) {
		if (buf[i] != 0xff) {
			ret++;
		}
	}
	if (!ret) {
		ets_printf("OK\n");
	} else {
		ets_printf("ERR\n");
		return -1;
	}

	/* Write pattern to flash page */
	for (i = 0; i < PAGE_SIZE; i++) {
		buf[i] = ASCII_PATTERN(i);
	}
	ets_printf("Step 2: Write\n* Write ascii pattern..");
	if (!flash_write(flash_dev, TEST_PARTITION_ADDRESS, buf, PAGE_SIZE)) {
		ets_printf("OK\n");
	} else {
		ets_printf("ERR\n");
		return -1;
	}

	/* Verify written pattern */
	ets_printf("Step 3: Verify\n* Verifing pattern.. ");
	memset(buf, 0, sizeof(buf));
	if (flash_read(flash_dev, TEST_PARTITION_ADDRESS, buf, PAGE_SIZE)) {
		ets_printf("ERR\n");
		return -1;
	}
	for (i = 0, ret = 0; i < PAGE_SIZE; i++) {
		if (ASCII_PATTERN(i) != buf[i]) {
			ret++;
		}
	}
	if (!ret) {
		ets_printf("OK\n");
	} else {
		ets_printf("ERR\n");
		return -1;
	}

	return 0;
}
