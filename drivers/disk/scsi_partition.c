/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/disk/scsi_partition.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/storage/disk_partition.h>

struct scsi_partition_io {
	struct scsi_device *sdev;
};

static int scsi_partition_read_sector(void *ctx, uint64_t lba, void *buf, uint32_t bytes)
{
	struct scsi_partition_io *io = ctx;

	return scsi_io_read(io->sdev, lba, 1U, buf, bytes);
}

static int scsi_partition_geo(const struct scsi_device *sdev, struct disk_partition_geo *geo)
{
	if (sdev == NULL || geo == NULL) {
		return -EINVAL;
	}

	geo->block_size = sdev->block_size;
	geo->sector_count = sdev->block_count;
	return 0;
}

int scsi_partition_walk(struct scsi_device *sdev, struct disk_partition_table *table)
{
	struct disk_partition_geo geo;
	struct scsi_partition_io io;
	int ret;

	if (table == NULL) {
		return -EINVAL;
	}

	ret = scsi_partition_geo(sdev, &geo);
	if (ret != 0) {
		return ret;
	}

	io.sdev = sdev;

	return disk_partition_walk(&geo, scsi_partition_read_sector, &io, table);
}

int scsi_partition_find_fat_volume(struct scsi_device *sdev, uint64_t *lba_offset,
				   uint64_t *sector_count)
{
	struct disk_partition_geo geo;
	struct scsi_partition_io io;
	int ret;

	if (lba_offset == NULL || sector_count == NULL) {
		return -EINVAL;
	}

	ret = scsi_partition_geo(sdev, &geo);
	if (ret != 0) {
		return ret;
	}

	io.sdev = sdev;

	return disk_partition_find_fat_volume(&geo, scsi_partition_read_sector, &io, lba_offset,
					      sector_count);
}

int scsi_partition_find_ext2_volume(struct scsi_device *sdev, uint64_t *lba_offset,
				    uint64_t *sector_count)
{
	struct disk_partition_geo geo;
	struct scsi_partition_io io;
	int ret;

	if (lba_offset == NULL || sector_count == NULL) {
		return -EINVAL;
	}

	ret = scsi_partition_geo(sdev, &geo);
	if (ret != 0) {
		return ret;
	}

	io.sdev = sdev;

	return disk_partition_find_ext2_volume(&geo, scsi_partition_read_sector, &io, lba_offset,
					       sector_count);
}
