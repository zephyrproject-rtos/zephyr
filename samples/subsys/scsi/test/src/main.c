/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/disk/scsi_partition.h>
#include <zephyr/kernel.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/scsi/scsi_cmd.h>
#include <zephyr/sys/byteorder.h>

#define SECTOR_SIZE  512U
#define DISK_SECTORS 12288U

struct mock_scsi_ctx {
	uint8_t image[DISK_SECTORS * SECTOR_SIZE];
	struct scsi_device sdev;
	struct device dev;
};

static struct mock_scsi_ctx mock;

static int mock_scsi_exec(const struct device *dev, struct scsi_xfer *xfer)
{
	struct mock_scsi_ctx *ctx = CONTAINER_OF(dev, struct mock_scsi_ctx, dev);

	if (xfer->cdb[0] == SCSI_OPCODE_READ_10 && xfer->dir == SCSI_DATA_READ &&
	    xfer->data != NULL) {
		const uint32_t lba = sys_get_be32(&xfer->cdb[2]);
		const uint32_t blocks = sys_get_be16(&xfer->cdb[7]);
		const uint32_t len = blocks * SECTOR_SIZE;

		if (lba + blocks > DISK_SECTORS || xfer->data_len < len) {
			return -EINVAL;
		}

		memcpy(xfer->data, &ctx->image[lba * SECTOR_SIZE], len);
	}

	xfer->status = SCSI_STATUS_GOOD;
	return 0;
}

static const struct scsi_driver_api mock_scsi_api = {
	.exec = mock_scsi_exec,
};

static void mock_mbr_fat(struct mock_scsi_ctx *ctx)
{
	uint8_t *mbr = &ctx->image[0];
	uint8_t *bpb = &ctx->image[4096U * SECTOR_SIZE];

	mbr[510] = 0x55U;
	mbr[511] = 0xAAU;
	mbr[0x1BE + 4] = 0x0CU;
	sys_put_le32(4096U, &mbr[0x1BE + 8]);
	sys_put_le32(8192U, &mbr[0x1BE + 12]);

	bpb[0] = 0xEB;
	memcpy(&bpb[82], "FAT32   ", 8);
	bpb[510] = 0x55U;
	bpb[511] = 0xAAU;
}

int main(void)
{
	struct scsi_xfer xfer = {0};
	uint64_t lba_off;
	uint64_t sec_count;
	int ret;

	printk("SCSI mid-layer sample\n");

	ret = scsi_cmd_test_unit_ready(&xfer);
	if (ret != 0) {
		printk("scsi_cmd_test_unit_ready failed: %d\n", ret);
		return 1;
	}
	printk("CDB builders OK\n");

	memset(&mock, 0, sizeof(mock));
	mock_mbr_fat(&mock);
	ret = scsi_device_init(&mock.sdev, &mock.dev, &mock_scsi_api, 0U);
	if (ret != 0) {
		printk("scsi_device_init failed: %d\n", ret);
		return 1;
	}
	mock.sdev.block_size = SECTOR_SIZE;
	mock.sdev.block_count = DISK_SECTORS;
	mock.sdev.state = SCSI_DEV_READY;

	ret = scsi_partition_find_fat_volume(&mock.sdev, &lba_off, &sec_count);
	if (ret != 0) {
		printk("scsi_partition_find_fat_volume failed: %d\n", ret);
		return 1;
	}

	printk("FAT volume LBA %llu sectors %llu\n", lba_off, sec_count);

	return 0;
}
