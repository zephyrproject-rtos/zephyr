/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kvss/zms.h>

#define ZMS_PARTITION        storage_partition
#define ZMS_PARTITION_DEVICE PARTITION_DEVICE(ZMS_PARTITION)
#define ZMS_PARTITION_OFFSET PARTITION_OFFSET(ZMS_PARTITION)

static struct zms_fs fs;

void init_storage(void)
{
	int rc;
	struct flash_pages_info info;

	fs.flash_device = ZMS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device %s is not ready\n", fs.flash_device->name);
		return;
	}

	fs.offset = ZMS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		printk("Unable to get page info, rc=%d\n", rc);
		return;
	}

	fs.sector_size = info.size;
	fs.sector_count = 3U;  /* Adjust based on your partition size */

	rc = zms_mount(&fs);
	if (rc) {
		printk("ZMS mount failed: %d\n", rc);
		return;
	}

	printk("ZMS mounted successfully\n");
}

void read_data(void)
{
	uint8_t buf[128];
	ssize_t len;

	/* Read ID 2 */
	len = zms_read(&fs, 2, buf, sizeof(buf));
	if (len > 0) {
		printk("ID 2: ");
		for (int i = 0; i < len; i++) {
			printk("%c", buf[i]);
		}
		printk("\n");
	}
}

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	init_storage();
	read_data();

	return 0;
}
