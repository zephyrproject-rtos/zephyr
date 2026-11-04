/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Thin SCSI initiator glue: sdev geometry + scsi_io_read -> disk_partition_walk.
 * No partition policy and no disk_access registration.
 */

#include <errno.h>

#include <zephyr/drivers/disk/scsi_partition.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/storage/disk_partition.h>

struct scsi_partition_io {
	struct scsi_device *sdev;
};

static int scsi_partition_check_sdev(const struct scsi_device *sdev)
{
	if (sdev == NULL) {
		return -EINVAL;
	}

	if (sdev->state == SCSI_DEV_REMOVED) {
		return -ENODEV;
	}

	if (sdev->state == SCSI_DEV_MEDIA_CHANGED || sdev->block_size == 0U ||
	    sdev->block_count == 0U) {
		return -ENODEV;
	}

	return 0;
}

static int scsi_partition_read_sector(void *ctx, uint64_t lba, void *buf, uint32_t bytes)
{
	struct scsi_partition_io *io = ctx;
	int ret;

	ret = scsi_partition_check_sdev(io->sdev);
	if (ret != 0) {
		return ret;
	}

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

	ret = scsi_partition_check_sdev(sdev);
	if (ret != 0) {
		return ret;
	}

	ret = scsi_partition_geo(sdev, &geo);
	if (ret != 0) {
		return ret;
	}

	io.sdev = sdev;

	return disk_partition_walk(&geo, scsi_partition_read_sector, &io, table);
}
