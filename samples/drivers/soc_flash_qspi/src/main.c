/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>

#define OFFSET_PAGE    0x00
#define QSPI_NUM_PAGES 10

#define QSPI_DEV       DEVICE_DT_GET(DT_ALIAS(qspi));

int main(void)
{
	const struct device *qspi_dev;
	struct flash_pages_info page;
	size_t flash_block_size;
	size_t total_pages;
	int ret;
	uint8_t *w_Page_buffer;
	uint8_t *r_Page_buffer;
	uint8_t page_data = 0;
	const struct flash_parameters *flash_cad_parameters;

	printk("QSPI flash driver test sample\n");

	qspi_dev = QSPI_DEV;

	if (!device_is_ready(qspi_dev)) {
		printk("QSPI flash driver is not ready\n");
		return -ENODEV;
	}

	flash_cad_parameters = flash_get_parameters(qspi_dev);
	printk("QSPI flash device block size = %lx\n", flash_cad_parameters->write_block_size);

	total_pages = flash_get_page_count(qspi_dev);
	printk("QSPI flash number of pages = %lx\n", total_pages);

	flash_block_size = flash_get_write_block_size(qspi_dev);
	printk("QSPI flash driver block size %lx\n", flash_block_size);

	ret = flash_get_page_info_by_offs(qspi_dev, 0x00, &page);
	if (ret < 0) {
		printk("QSPI flash driver page info error\n");
		return ret;
	}
	printk("The Page size of %lx\n", page.size);

	w_Page_buffer = (uint8_t *)k_malloc(page.size * QSPI_NUM_PAGES);

	if (w_Page_buffer != NULL) {
		for (int index = 0; index < page.size * QSPI_NUM_PAGES; index++) {
			w_Page_buffer[index] = (page_data++ % 255);
		}
	} else {
		printk("Write memory not allocated\n");
		return -ENOSR;
	}

	r_Page_buffer = (uint8_t *)k_malloc(page.size * QSPI_NUM_PAGES);
	if (r_Page_buffer != NULL) {
		memset(r_Page_buffer, 0x55, page.size * QSPI_NUM_PAGES);
	} else {
		printk("Read memory not allocated\n");
		k_free(w_Page_buffer);
		return -ENOSR;
	}

	ret = flash_erase(qspi_dev, OFFSET_PAGE, flash_block_size);
	if (ret < 0) {
		printk("QSPI flash driver read Error\n");
		k_free(w_Page_buffer);
		k_free(r_Page_buffer);
		return ret;
	}
	printk("QSPI flash driver data erase successful....\n");

	ret = flash_write(qspi_dev, OFFSET_PAGE, w_Page_buffer, page.size * QSPI_NUM_PAGES);
	if (ret < 0) {
		printk("QSPI flash driver write error\n");
		k_free(w_Page_buffer);
		k_free(r_Page_buffer);
		return ret;
	}
	printk("QSPI flash driver write completed....\n");

	ret = flash_read(qspi_dev, OFFSET_PAGE, r_Page_buffer, page.size * QSPI_NUM_PAGES);
	if (ret < 0) {
		printk("QSPI flash driver read error\n");
		k_free(w_Page_buffer);
		k_free(r_Page_buffer);
		return ret;
	}
	printk("QSPI flash driver read completed....\n");

	ret = memcmp(w_Page_buffer, r_Page_buffer, page.size * QSPI_NUM_PAGES);

	if (ret < 0) {
		printk("QSPI flash driver read not match\n");
		k_free(w_Page_buffer);
		k_free(r_Page_buffer);
		return ret;
	}
	printk("QSPI flash driver read verified\n");

	ret = flash_erase(qspi_dev, OFFSET_PAGE, flash_block_size);

	if (ret < 0) {
		printk("QSPI flash driver read Error\n");
		k_free(w_Page_buffer);
		k_free(r_Page_buffer);
		return ret;
	}
	printk("QSPI flash driver data erase successful....\n");

	ret = flash_read(qspi_dev, OFFSET_PAGE, r_Page_buffer, page.size * QSPI_NUM_PAGES);
	if (ret != 0) {
		printk("QSPI flash driver read error\n");
		k_free(w_Page_buffer);
		k_free(r_Page_buffer);
		return ret;
	}
	memset(w_Page_buffer, 0xFF, page.size * QSPI_NUM_PAGES);

	ret = memcmp(w_Page_buffer, r_Page_buffer, page.size * QSPI_NUM_PAGES);

	if (ret < 0) {
		printk("QSPI flash driver read not match\n");
		k_free(w_Page_buffer);
		k_free(r_Page_buffer);
		return ret;
	}
	printk("QSPI flash driver read verified after erase....\n");
	printk("QSPI flash driver test sample completed....\n");

	return 0;
}
