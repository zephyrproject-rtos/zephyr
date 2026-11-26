/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/disk/scsi_disk.h>
#include <zephyr/drivers/disk/scsi_partition.h>
#include <zephyr/scsi/scsi.h>
#include <zephyr/storage/disk_partition.h>
#include <zephyr/scsi/scsi_cmd.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define SECTOR_SIZE  512U
#define DISK_SECTORS 12288U
#define DISK_NAME    "SCSI0"
#define PART_START   4096U
#define PART_SECTORS 8192U

struct mock_scsi_ctx {
	uint8_t image[DISK_SECTORS * SECTOR_SIZE];
	struct scsi_device sdev;
	struct device dev;
	struct scsi_disk disk;
	bool fail_read;
};

static struct mock_scsi_ctx g_mock;

static int mock_scsi_exec(const struct device *dev, struct scsi_xfer *xfer)
{
	struct mock_scsi_ctx *ctx = CONTAINER_OF(dev, struct mock_scsi_ctx, dev);

	if (ctx->fail_read && xfer->cdb[0] == SCSI_OPCODE_READ_10) {
		return -EIO;
	}

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

	if (xfer->cdb[0] == SCSI_OPCODE_WRITE_10 && xfer->dir == SCSI_DATA_WRITE &&
	    xfer->data != NULL) {
		const uint32_t lba = sys_get_be32(&xfer->cdb[2]);
		const uint32_t blocks = sys_get_be16(&xfer->cdb[7]);
		const uint32_t len = blocks * SECTOR_SIZE;

		if (lba + blocks > DISK_SECTORS || xfer->data_len < len) {
			return -EINVAL;
		}

		memcpy(&ctx->image[lba * SECTOR_SIZE], xfer->data, len);
	}

	xfer->status = SCSI_STATUS_GOOD;
	return 0;
}

static const struct scsi_driver_api mock_scsi_api = {
	.exec = mock_scsi_exec,
};

static void mock_mbr_partition(struct mock_scsi_ctx *ctx)
{
	uint8_t *mbr = &ctx->image[0];

	mbr[510] = 0x55U;
	mbr[511] = 0xAAU;
	mbr[0x1BE + 4] = 0x0CU;
	sys_put_le32(PART_START, &mbr[0x1BE + 8]);
	sys_put_le32(PART_SECTORS, &mbr[0x1BE + 12]);

	ctx->image[PART_START * SECTOR_SIZE] = 0xA5U;
}

static void mock_setup(struct mock_scsi_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	mock_mbr_partition(ctx);
	zassert_ok(scsi_device_init(&ctx->sdev, &ctx->dev, &mock_scsi_api, 0U));
	ctx->sdev.block_size = SECTOR_SIZE;
	ctx->sdev.block_count = DISK_SECTORS;
	ctx->sdev.state = SCSI_DEV_READY;
}

ZTEST(scsi_disk, test_scsi_partition_walk_mbr)
{
	struct disk_partition_table table;
	int ret;

	mock_setup(&g_mock);

	ret = scsi_partition_walk(&g_mock.sdev, &table);
	zassert_ok(ret);
	zassert_equal(DISK_PARTITION_SCHEME_MBR, table.scheme);
	zassert_equal(1, table.entry_count);
	zassert_equal(PART_START, table.entries[0].start_lba);
	zassert_equal(PART_SECTORS, table.entries[0].nr_sectors);
	zassert_equal(0x0CU, table.entries[0].mbr_type);
}

ZTEST(scsi_disk, test_disk_register_read_write_slice)
{
	struct disk_partition_table table;
	uint8_t buf[SECTOR_SIZE];
	int ret;

	mock_setup(&g_mock);

	ret = scsi_partition_walk(&g_mock.sdev, &table);
	zassert_ok(ret);
	zassert_equal(1, table.entry_count);

	g_mock.disk.lba_offset = table.entries[0].start_lba;
	g_mock.disk.sector_count = table.entries[0].nr_sectors;

	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_read(DISK_NAME, buf, 0U, 1U);
	zassert_ok(ret);
	zassert_equal(0xA5, buf[0]);

	buf[0] = 0x5AU;
	ret = disk_access_write(DISK_NAME, buf, 1U, 1U);
	zassert_ok(ret);

	memset(buf, 0, sizeof(buf));
	ret = disk_access_read(DISK_NAME, buf, 1U, 1U);
	zassert_ok(ret);
	zassert_equal(0x5AU, buf[0]);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST(scsi_disk, test_slice_whole_lun)
{
	uint32_t sector_count;
	int ret;

	mock_setup(&g_mock);
	g_mock.disk.lba_offset = 0U;
	g_mock.disk.sector_count = 0U;

	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	zassert_ok(ret);
	zassert_equal(DISK_SECTORS, sector_count);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST(scsi_disk, test_slice_remaining_lun)
{
	uint32_t sector_count;
	uint8_t buf[SECTOR_SIZE];
	int ret;

	mock_setup(&g_mock);
	g_mock.disk.lba_offset = PART_START;
	g_mock.disk.sector_count = 0U;

	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	zassert_ok(ret);
	zassert_equal(DISK_SECTORS - PART_START, sector_count);

	ret = disk_access_read(DISK_NAME, buf, sector_count - 1U, 1U);
	zassert_ok(ret);

	ret = disk_access_read(DISK_NAME, buf, sector_count, 1U);
	zassert_equal(-EINVAL, ret);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST(scsi_disk, test_slice_explicit_bounds)
{
	uint32_t sector_count;
	uint8_t buf[SECTOR_SIZE];
	int ret;

	mock_setup(&g_mock);
	g_mock.disk.lba_offset = PART_START;
	g_mock.disk.sector_count = PART_SECTORS;

	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	zassert_ok(ret);
	zassert_equal(PART_SECTORS, sector_count);

	ret = disk_access_read(DISK_NAME, buf, 0U, 1U);
	zassert_ok(ret);
	zassert_equal(0xA5, buf[0]);

	ret = disk_access_read(DISK_NAME, buf, PART_SECTORS - 1U, 1U);
	zassert_ok(ret);

	ret = disk_access_read(DISK_NAME, buf, PART_SECTORS, 1U);
	zassert_equal(-EINVAL, ret);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST(scsi_disk, test_register_rejects_offset_beyond_lun)
{
	mock_setup(&g_mock);
	g_mock.disk.lba_offset = DISK_SECTORS + 1U;
	g_mock.disk.sector_count = 0U;

	zassert_equal(-EINVAL, scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME));
}

ZTEST(scsi_disk, test_register_rejects_explicit_size_overflow)
{
	mock_setup(&g_mock);
	g_mock.disk.lba_offset = PART_START;
	g_mock.disk.sector_count = DISK_SECTORS;

	zassert_equal(-EINVAL, scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME));
}

ZTEST(scsi_disk, test_register_rejects_empty_remaining_span)
{
	mock_setup(&g_mock);
	g_mock.disk.lba_offset = DISK_SECTORS;
	g_mock.disk.sector_count = 0U;

	zassert_equal(-EINVAL, scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME));
}

ZTEST(scsi_disk, test_register_rejects_overflow_metadata)
{
	mock_setup(&g_mock);
	g_mock.disk.lba_offset = UINT64_MAX;
	g_mock.disk.sector_count = 1U;

	zassert_equal(-EINVAL, scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME));
}

ZTEST(scsi_disk, test_disk_geometry_ioctl)
{
	uint32_t sector_count;
	uint32_t sector_size;
	int ret;

	mock_setup(&g_mock);
	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	zassert_ok(ret);
	zassert_equal(DISK_SECTORS, sector_count);

	ret = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
	zassert_ok(ret);
	zassert_equal(SECTOR_SIZE, sector_size);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST(scsi_disk, test_write_protected)
{
	uint8_t buf[SECTOR_SIZE] = {0x01U};
	int ret;

	mock_setup(&g_mock);
	g_mock.sdev.write_protected = true;
	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_write(DISK_NAME, buf, 0U, 1U);
	zassert_equal(-EROFS, ret);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST(scsi_disk, test_read_past_capacity)
{
	uint8_t buf[SECTOR_SIZE];
	int ret;

	mock_setup(&g_mock);
	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_read(DISK_NAME, buf, DISK_SECTORS, 1U);
	zassert_equal(-EINVAL, ret);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST(scsi_disk, test_transport_error)
{
	uint8_t buf[SECTOR_SIZE];
	int ret;

	mock_setup(&g_mock);
	g_mock.fail_read = true;
	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_read(DISK_NAME, buf, 0U, 1U);
	zassert_equal(-EIO, ret);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST(scsi_disk, test_register_rejects_removed)
{
	mock_setup(&g_mock);
	scsi_device_mark_removed(&g_mock.sdev);
	zassert_equal(-ENODEV, scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME));
}

ZTEST(scsi_disk, test_sector_count_exceeds_uint32)
{
	uint32_t sector_count;
	int ret;

	mock_setup(&g_mock);
	g_mock.sdev.block_count = (uint64_t)UINT32_MAX + 1U;
	ret = scsi_disk_register(&g_mock.disk, &g_mock.sdev, DISK_NAME);
	zassert_ok(ret);

	ret = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	zassert_equal(-ENOTSUP, ret);

	scsi_disk_unregister(&g_mock.disk);
}

ZTEST_SUITE(scsi_disk, NULL, NULL, NULL, NULL, NULL);
