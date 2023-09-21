/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>

#include "utils.h"

static int sectors_info(const char *disk, uint32_t *ss, uint32_t *sc)
{
	int rc;

	rc = disk_access_ioctl(disk, DISK_IOCTL_GET_SECTOR_COUNT, sc);
	if (rc < 0) {
		TC_PRINT("Disk access (sector count) error: %d", rc);
		return rc;
	}

	rc = disk_access_ioctl(disk, DISK_IOCTL_GET_SECTOR_SIZE, ss);
	if (rc < 0) {
		TC_PRINT("Disk access (sector size) error: %d", rc);
		return rc;
	}

	return 0;
}

int wipe_partition(uintptr_t id)
{
	int rc;
	const char *name = (const char *)id;
	uint32_t sector_count, sector_size;

	TC_PRINT("Wiping %s\n", name);

	rc = disk_access_init(name);
	if (rc < 0) {
		return rc;
	}

	rc = sectors_info(name, &sector_size, &sector_count);
	if (rc < 0) {
		return rc;
	}

	uint8_t zeros[sector_size];

	memset(zeros, 0, sector_size);

	/* Superblock is located at offset 1024B and has size 1024B */
	uint32_t start_sector = CONFIG_EXT2_DISK_STARTING_SECTOR + 1024 / sector_size;
	uint32_t num_sectors = 1024 / sector_size + (1024 % sector_size != 0);

	for (uint32_t i = 0; i < num_sectors; i++) {
		rc = disk_access_write(name, zeros, start_sector + i, 1);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

size_t get_partition_size(uintptr_t id)
{
	const char *name = (const char *)id;
	uint32_t sector_count, sector_size;

	sectors_info(name, &sector_size, &sector_count);

	return sector_size * sector_count;
}
