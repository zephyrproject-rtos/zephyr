/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>

#include "ext2.h"
#include "ext2_struct.h"

LOG_MODULE_DECLARE(ext2);

static struct disk_data {
	const char *name;
	uint32_t sector_size;
	uint32_t sector_count;
} disk_data;

static int64_t disk_access_device_size(struct ext2_data *fs)
{
	struct disk_data *disk = fs->backend;

	return (uint64_t)disk->sector_count * (uint64_t)disk->sector_size;
}

static int64_t disk_access_write_size(struct ext2_data *fs)
{
	struct disk_data *disk = fs->backend;

	return disk->sector_size;
}

static int disk_read(const char *disk, uint8_t *buf, uint32_t start, uint32_t num)
{
	int rc, loop = 0;

	do {
		rc = disk_access_ioctl(disk, DISK_IOCTL_CTRL_SYNC, NULL);
		if (rc == 0) {
			rc = disk_access_read(disk, buf, start, num);
			LOG_DBG("disk read: (start:%d, num:%d) (ret: %d)", start, num, rc);
		}
	} while ((rc == -EBUSY) && (loop++ < 16));
	return rc;
}

static int disk_write(const char *disk, const uint8_t *buf, uint32_t start, uint32_t num)
{
	int rc, loop = 0;

	do {
		rc = disk_access_ioctl(disk, DISK_IOCTL_CTRL_SYNC, NULL);
		if (rc == 0) {
			rc = disk_access_write(disk, buf, start, num);
			LOG_DBG("disk write: (start:%d, num:%d) (ret: %d)", start, num, rc);
		}
	} while ((rc == -EBUSY) && (loop++ < 16));
	return rc;
}

static int disk_prepare_range(struct disk_data *disk, uint32_t addr, uint32_t size,
		uint32_t *s_start, uint32_t *s_count)
{
	*s_start = CONFIG_EXT2_DISK_STARTING_SECTOR + addr / disk->sector_size;
	*s_count = size / disk->sector_size;

	LOG_DBG("addr:0x%x size:0x%x -> sector_start:%d sector_count:%d",
			addr, size, *s_start, *s_count);

	/* Check for overflow. */
	if (*s_count > UINT32_MAX - *s_start) {
		LOG_ERR("Requested range (%d:+%d) can't be accessed due to overflow.",
				*s_start, *s_count);
		return -ENOSPC;
	}

	/* Cannot read or write outside the disk. */
	if (*s_start + *s_count > disk->sector_count) {
		LOG_ERR("Requested sectors: %d-%d are outside of disk (num_sectors: %d)",
			*s_start, *s_start + *s_count, disk->sector_count);
		return -ENOSPC;
	}
	return 0;
}

static int disk_access_read_block(struct ext2_data *fs, void *buf, uint32_t block)
{
	int rc;
	struct disk_data *disk = fs->backend;
	uint32_t sector_start, sector_count;

	rc = disk_prepare_range(disk, block * fs->block_size, fs->block_size,
			&sector_start, &sector_count);
	if (rc < 0) {
		return rc;
	}
	return disk_read(disk->name, buf, sector_start, sector_count);
}

static int disk_access_write_block(struct ext2_data *fs, const void *buf, uint32_t block)
{
	int rc;
	struct disk_data *disk = fs->backend;
	uint32_t sector_start, sector_count;

	rc = disk_prepare_range(disk, block * fs->block_size, fs->block_size,
			&sector_start, &sector_count);
	if (rc < 0) {
		return rc;
	}
	return disk_write(disk->name, buf, sector_start, sector_count);
}

static int disk_access_read_superblock(struct ext2_data *fs, struct ext2_disk_superblock *sb)
{
	int rc;
	struct disk_data *disk = fs->backend;
	uint32_t sector_start, sector_count;

	rc = disk_prepare_range(disk, EXT2_SUPERBLOCK_OFFSET, sizeof(struct ext2_disk_superblock),
			&sector_start, &sector_count);
	if (rc < 0) {
		return rc;
	}
	return disk_read(disk->name, (uint8_t *)sb, sector_start, sector_count);
}

static int disk_access_sync(struct ext2_data *fs)
{
	struct disk_data *disk = fs->backend;

	LOG_DBG("Sync disk %s", disk->name);
	return disk_access_ioctl(disk->name, DISK_IOCTL_CTRL_SYNC, NULL);
}

static const struct ext2_backend_ops disk_access_ops = {
	.get_device_size = disk_access_device_size,
	.get_write_size = disk_access_write_size,
	.read_block = disk_access_read_block,
	.write_block = disk_access_write_block,
	.read_superblock = disk_access_read_superblock,
	.sync = disk_access_sync,
};

int ext2_init_disk_access_backend(struct ext2_data *fs, const void *storage_dev, int flags)
{
	int rc;
	uint32_t sector_size, sector_count;
	const char *name = (const char *)storage_dev;

	rc = disk_access_init(name);
	if (rc < 0) {
		LOG_ERR("FAIL: unable to find disk %s: %d\n", name, rc);
		return rc;
	}

	rc = disk_access_ioctl(name, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	if (rc < 0) {
		LOG_ERR("Disk access (sector count) error: %d", rc);
		return rc;
	}

	rc = disk_access_ioctl(name, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
	if (rc < 0) {
		LOG_ERR("Disk access (sector size) error: %d", rc);
		return rc;
	}

	disk_data = (struct disk_data) {
		.name = storage_dev,
		.sector_size = sector_size,
		.sector_count = sector_count,
	};

	fs->backend = &disk_data;
	fs->backend_ops = &disk_access_ops;
	return 0;
}
