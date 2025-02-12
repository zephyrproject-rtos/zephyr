/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/pm/device.h>

/* Set to 1 to test the chip erase functionality. Please be aware that this
 * operation takes quite a while (it depends on the chip size, but can easily
 * take tens of seconds).
 * Note - erasing of the test region or whole chip is performed only when
 *        CONFIG_SPI_FLASH_AT45_USE_READ_MODIFY_WRITE is not enabled.
 */
#define ERASE_WHOLE_CHIP    0

#define TEST_REGION_OFFSET  0xFE00
#define TEST_REGION_SIZE    0x400

static uint8_t write_buf[TEST_REGION_SIZE];
static uint8_t read_buf[TEST_REGION_SIZE];

int main(void)
{
	printk("DataFlash sample on %s\n", CONFIG_BOARD);

	const struct device *const flash_dev = DEVICE_DT_GET_ONE(atmel_at45);
	int i;
	int err;
	uint8_t data;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_info pages_info;
	size_t page_count, chip_size;
#endif

	if (!device_is_ready(flash_dev)) {
		printk("%s: device not ready.\n", flash_dev->name);
		return 0;
	}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
	page_count = flash_get_page_count(flash_dev);
	(void)flash_get_page_info_by_idx(flash_dev, 0, &pages_info);
	chip_size = page_count * pages_info.size;
	printk("Using %s, chip size: %u bytes (page: %u)\n",
	       flash_dev->name, chip_size, pages_info.size);
#endif

	printk("Reading the first byte of the test region ... ");
	err = flash_read(flash_dev, TEST_REGION_OFFSET, &data, 1);
	if (err != 0) {
		printk("FAILED\n");
		return 0;
	}

	printk("OK\n");

	++data;
	printk("Preparing test content starting with 0x%02X.\n", data);
	for (i = 0; i < TEST_REGION_SIZE; ++i) {
		write_buf[i] = (uint8_t)(data + i);
	}

#ifndef CONFIG_SPI_FLASH_AT45_USE_READ_MODIFY_WRITE
	if (ERASE_WHOLE_CHIP) {
#ifdef CONFIG_FLASH_PAGE_LAYOUT
		printk("Erasing the whole chip... ");
		err = flash_erase(flash_dev, 0, chip_size);
#else
		#error To full chip erase you need enable flash page layout
#endif
	} else {
		printk("Erasing the test region... ");
		err = flash_erase(flash_dev,
				  TEST_REGION_OFFSET, TEST_REGION_SIZE);
	}

	if (err != 0) {
		printk("FAILED\n");
		return 0;
	}

	printk("OK\n");

	printk("Checking if the test region is erased... ");
	err = flash_read(flash_dev, TEST_REGION_OFFSET,
			 read_buf, TEST_REGION_SIZE);
	if (err != 0) {
		printk("FAILED\n");
		return 0;
	}

	for (i = 0; i < TEST_REGION_SIZE; ++i) {
		if (read_buf[i] != 0xFF) {
			printk("\nERROR at read_buf[%d]: "
			       "expected 0x%02X, got 0x%02X\n",
			       i, 0xFF, read_buf[i]);
			return 0;
		}
	}

	printk("OK\n");
#endif /* !CONFIG_SPI_FLASH_AT45_USE_READ_MODIFY_WRITE */

	printk("Writing the first half of the test region... ");
	err = flash_write(flash_dev, TEST_REGION_OFFSET,
			  write_buf,  TEST_REGION_SIZE/2);
	if (err != 0) {
		printk("FAILED\n");
		return 0;
	}

	printk("OK\n");

	printk("Writing the second half of the test region... ");
	err = flash_write(flash_dev, TEST_REGION_OFFSET + TEST_REGION_SIZE/2,
			  &write_buf[TEST_REGION_SIZE/2], TEST_REGION_SIZE/2);
	if (err != 0) {
		printk("FAILED\n");
		return 0;
	}

	printk("OK\n");

	printk("Reading the whole test region... ");
	err = flash_read(flash_dev, TEST_REGION_OFFSET,
			 read_buf, TEST_REGION_SIZE);
	if (err != 0) {
		printk("FAILED\n");
		return 0;
	}

	printk("OK\n");

	printk("Checking the read content... ");
	for (i = 0; i < TEST_REGION_SIZE; ++i) {
		if (read_buf[i] != write_buf[i]) {
			printk("\nERROR at read_buf[%d]: "
			       "expected 0x%02X, got 0x%02X\n",
			       i, write_buf[i], read_buf[i]);
			return 0;
		}
	}

	printk("OK\n");

#if defined(CONFIG_PM_DEVICE)
	printk("Putting the flash device into suspended state... ");
	err = pm_device_action_run(flash_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err != 0) {
		printk("FAILED\n");
		return 0;
	}

	printk("OK\n");
#endif

	k_sleep(K_FOREVER);
	return 0;
}
