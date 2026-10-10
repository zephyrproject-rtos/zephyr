/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "usbd_msc_scsi.h"

#define SECTOR_SIZE  512U
#define SECTOR_COUNT 8U

static struct scsi_ctx ctx;
static uint8_t buffer[CONFIG_USBD_MSC_SCSI_BUFFER_SIZE];
static int disk_status;
static uint32_t write_count;

static int test_disk_init(struct disk_info *disk)
{
	return 0;
}

static int test_disk_status(struct disk_info *disk)
{
	return disk_status;
}

static int test_disk_read(struct disk_info *disk, uint8_t *data, uint32_t sector, uint32_t count)
{
	zassert_equal(sector, 0U);
	zassert_equal(count, 1U);
	memset(data, 0xa5, SECTOR_SIZE);
	return 0;
}

static int test_disk_write(struct disk_info *disk, const uint8_t *data, uint32_t sector,
			   uint32_t count)
{
	write_count++;
	return 0;
}

static int test_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *data)
{
	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)data = SECTOR_COUNT;
		return 0;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)data = SECTOR_SIZE;
		return 0;
	case DISK_IOCTL_CTRL_INIT:
	case DISK_IOCTL_CTRL_SYNC:
		return 0;
	default:
		return -EINVAL;
	}
}

static const struct disk_operations disk_ops = {
	.init = test_disk_init,
	.status = test_disk_status,
	.read = test_disk_read,
	.write = test_disk_write,
	.ioctl = test_disk_ioctl,
};

static struct disk_info disk = {
	.name = "scsi_test",
	.ops = &disk_ops,
};

static void *setup(void)
{
	zassert_ok(disk_access_register(&disk));
	zassert_ok(disk_access_ioctl(disk.name, DISK_IOCTL_CTRL_INIT, NULL));
	return NULL;
}

static void before(void *fixture)
{
	disk_status = DISK_STATUS_OK;
	write_count = 0U;
	memset(buffer, 0, sizeof(buffer));
	scsi_init(&ctx, disk.name, "Zephyr", "SCSI test", "0.01");
}

static void check_read(void)
{
	const uint8_t cmd[] = {0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0};

	zassert_equal(scsi_cmd(&ctx, cmd, sizeof(cmd), buffer), 0U);
	zassert_equal(scsi_cmd_get_status(&ctx), GOOD);
	zassert_equal(scsi_cmd_remaining_data_len(&ctx), SECTOR_SIZE);
	zassert_equal(scsi_read_data(&ctx, buffer, sizeof(buffer)), SECTOR_SIZE);
	for (size_t i = 0; i < SECTOR_SIZE; i++) {
		zassert_equal(buffer[i], 0xa5);
	}
}

ZTEST(msc_scsi, test_write_protected_disk_is_ready)
{
	const uint8_t ready[] = {0, 0, 0, 0, 0, 0};
	const uint8_t capacity[] = {0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	disk_status = DISK_STATUS_WR_PROTECT;
	zassert_equal(scsi_cmd(&ctx, ready, sizeof(ready), buffer), 0U);
	zassert_equal(scsi_cmd_get_status(&ctx), GOOD);
	zassert_equal(scsi_cmd(&ctx, capacity, sizeof(capacity), buffer), 8U);
	zassert_equal(scsi_cmd_get_status(&ctx), GOOD);
	zassert_equal(sys_get_be32(buffer), SECTOR_COUNT - 1U);
	zassert_equal(sys_get_be32(buffer + 4), SECTOR_SIZE);
	check_read();
}

ZTEST(msc_scsi, test_mode_sense_write_protect)
{
	const uint8_t mode6[] = {0x1a, 0, 0x3f, 0, 4, 0};
	const uint8_t mode10[] = {0x5a, 0, 0x3f, 0, 0, 0, 0, 0, 8, 0};
	const int statuses[] = {DISK_STATUS_OK, DISK_STATUS_WR_PROTECT, DISK_STATUS_OK, -EIO};

	for (size_t i = 0; i < ARRAY_SIZE(statuses); i++) {
		uint8_t expected = statuses[i] == DISK_STATUS_WR_PROTECT ? BIT(7) : 0U;

		disk_status = statuses[i];
		zassert_equal(scsi_cmd(&ctx, mode6, sizeof(mode6), buffer), 4U);
		zassert_equal(scsi_cmd_get_status(&ctx), GOOD);
		zassert_equal(buffer[2], expected);
		zassert_equal(scsi_cmd(&ctx, mode10, sizeof(mode10), buffer), 8U);
		zassert_equal(scsi_cmd_get_status(&ctx), GOOD);
		zassert_equal(buffer[3], expected);
	}
}

ZTEST(msc_scsi, test_write_protected_write_reports_sense)
{
	const uint8_t write[] = {0x2a, 0, 0, 0, 0, 0, 0, 0, 1, 0};
	const uint8_t sense[] = {0x03, 0, 0, 0, 18, 0};

	disk_status = DISK_STATUS_WR_PROTECT;
	zassert_equal(scsi_cmd(&ctx, write, sizeof(write), buffer), 0U);
	zassert_equal(scsi_cmd_get_status(&ctx), CHECK_CONDITION);
	zassert_equal(scsi_cmd_remaining_data_len(&ctx), 0U);
	zassert_equal(scsi_write_data(&ctx, buffer, SECTOR_SIZE), 0U);
	zassert_equal(write_count, 0U);
	zassert_equal(scsi_cmd(&ctx, sense, sizeof(sense), buffer), 18U);
	zassert_equal(buffer[2] & 0x0f, DATA_PROTECT);
	zassert_equal(buffer[12], 0x27);
	zassert_equal(buffer[13], 0U);
	check_read();
}

ZTEST(msc_scsi, test_writable_disk)
{
	const uint8_t write[] = {0x2a, 0, 0, 0, 0, 0, 0, 0, 1, 0};

	zassert_equal(scsi_cmd(&ctx, write, sizeof(write), buffer), 0U);
	zassert_equal(scsi_cmd_get_status(&ctx), GOOD);
	zassert_equal(scsi_write_data(&ctx, buffer, SECTOR_SIZE), SECTOR_SIZE);
	zassert_equal(write_count, 1U);
	check_read();
}

ZTEST(msc_scsi, test_unavailable_disk)
{
	const uint8_t ready[] = {0, 0, 0, 0, 0, 0};
	const uint8_t capacity[] = {0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	const uint8_t read[] = {0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0};
	const uint8_t write[] = {0x2a, 0, 0, 0, 0, 0, 0, 0, 1, 0};
	const int statuses[] = {
		DISK_STATUS_UNINIT,
		DISK_STATUS_NOMEDIA,
		DISK_STATUS_UNINIT | DISK_STATUS_WR_PROTECT,
		DISK_STATUS_NOMEDIA | DISK_STATUS_WR_PROTECT,
		-EIO,
	};
	const struct {
		const uint8_t *cmd;
		size_t len;
	} commands[] = {
		{ready, sizeof(ready)},
		{capacity, sizeof(capacity)},
		{read, sizeof(read)},
		{write, sizeof(write)},
	};

	for (size_t i = 0; i < ARRAY_SIZE(statuses); i++) {
		disk_status = statuses[i];
		for (size_t j = 0; j < ARRAY_SIZE(commands); j++) {
			zassert_equal(scsi_cmd(&ctx, commands[j].cmd, commands[j].len, buffer), 0U);
			zassert_equal(scsi_cmd_get_status(&ctx), CHECK_CONDITION);
			zassert_equal(ctx.sense_key, NOT_READY);
		}
	}
	zassert_equal(write_count, 0U);
}

ZTEST_SUITE(msc_scsi, NULL, setup, before, NULL, NULL);
