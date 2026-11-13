/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/disk/scsi_partition.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/scsi/scsi_cmd.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define SECTOR_SIZE  512U
#define DISK_SECTORS 12288U

struct mock_scsi_ctx {
	uint8_t image[DISK_SECTORS * SECTOR_SIZE];
	struct scsi_device sdev;
	struct device dev;
};

static struct mock_scsi_ctx g_mock;

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

ZTEST(scsi_disk, test_scsi_partition_find_fat_volume)
{
	uint64_t lba_off;
	uint64_t sec_count;
	int ret;

	memset(&g_mock, 0, sizeof(g_mock));
	mock_mbr_fat(&g_mock);
	ret = scsi_device_init(&g_mock.sdev, &g_mock.dev, &mock_scsi_api, 0U);
	zassert_ok(ret);
	g_mock.sdev.block_size = SECTOR_SIZE;
	g_mock.sdev.block_count = DISK_SECTORS;
	g_mock.sdev.state = SCSI_DEV_READY;

	ret = scsi_partition_find_fat_volume(&g_mock.sdev, &lba_off, &sec_count);
	zassert_ok(ret);
	zassert_equal(4096U, lba_off);
	zassert_equal(8192U, sec_count);
}

ZTEST_SUITE(scsi_disk, NULL, NULL, NULL, NULL, NULL);
