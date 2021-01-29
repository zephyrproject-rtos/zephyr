/*
 * Copyright (c) 2019 Tavish Naruka <tavishnaruka@gmail.com>
 * Copyright (c) 2021 Filip Zawadiak <fzawadiak@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <device.h>
#include <disk/disk_access.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(main);

void main(void)
{
	static const char *disk_pdrv = "SDMMC";
	uint64_t memory_size_mb;
	uint32_t block_count;
	uint32_t block_size;

	if (disk_access_init(disk_pdrv) != 0) {
		LOG_ERR("Storage init ERROR!");
		return;
	}

	if (disk_access_ioctl(disk_pdrv,
			      DISK_IOCTL_GET_SECTOR_COUNT,
			      &block_count)) {
		LOG_ERR("Unable to get sector count");
		return;
	}
	printk("Block count %u\n", block_count);

	if (disk_access_ioctl(disk_pdrv,
			      DISK_IOCTL_GET_SECTOR_SIZE,
			      &block_size)) {
		LOG_ERR("Unable to get sector size");
		return;
	}
	printk("Sector size %u\n", block_size);

	memory_size_mb = (uint64_t)block_count * block_size;
	printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

	uint8_t buf[512];

	printk("Writing sectors");
	for (int i = 0; i < 100; i++) {
		memset(buf, i, sizeof(buf));
		if (disk_access_write(disk_pdrv, buf, i, 1)) {
			printk("E");
			continue;
		}
		printk(".");
	}
	printk("\nChecking sectors");

	for (int i = 0; i < 100; i++) {
		int j;

		if (disk_access_read(disk_pdrv, buf, i, 1)) {
			printk("E");
			continue;
		}

		for (j = 0; j < sizeof(buf); j++) {
			if (buf[j] != i) {
				printk("X");
				break;
			}
		}

		if (j == sizeof(buf))
			printk(".");
	}

	printk("\nErasing sectors");

	memset(buf, 0, sizeof(buf));
	for (int i = 0; i < 100; i++) {
		if (disk_access_write(disk_pdrv, buf, i, 1)) {
			printk("E");
			continue;
		}
		printk(".");
	}
	printk("\n");

	while (1) {
		k_sleep(K_MSEC(1000));
	}
}
