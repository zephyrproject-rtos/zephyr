/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/disk/virtual_fat_disk.h>
#include <zephyr/fs/virtual_fat.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_mock.h>

#include <errno.h>
#include <string.h>

#define TEST_DISK   DT_NODELABEL(test_disk)
#define DISK_DEV    DEVICE_DT_GET(TEST_DISK)
#define DISK_NAME   DT_PROP(TEST_DISK, disk_name)
#define SECTOR_SIZE DT_PROP(TEST_DISK, sector_size)

static void *virtual_fat_disk_setup(void)
{
	zassert_true(device_is_ready(DISK_DEV), "test disk device is not ready");
	zassert_ok(disk_access_init(DISK_NAME), "disk init failed");
	return NULL;
}

static void virtual_fat_disk_before(void *fixture)
{
	ARG_UNUSED(fixture);

	disk_register_virtual_files(DISK_DEV, NULL, 0);
	disk_register_new_file_write_cb(DISK_DEV, NULL, NULL);
}

ZTEST(virtual_fat_disk, test_status)
{
	zassert_equal(disk_access_status(DISK_NAME), DISK_STATUS_OK);

	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_CTRL_SYNC, NULL));
	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_CTRL_INIT, NULL));
	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_CTRL_DEINIT, NULL));
	zassert_equal(disk_access_ioctl(DISK_NAME, 0xFF, NULL), -EINVAL);
}

ZTEST(virtual_fat_disk, test_sector_size_and_count)
{
	zassert_equal(disk_access_status(DISK_NAME), DISK_STATUS_OK);

	uint8_t buf[SECTOR_SIZE];

	zassert_ok(disk_access_read(DISK_NAME, buf, 0, 1));

	const struct fat1x_BS *boot_sector = (const struct fat1x_BS *)buf;
	uint32_t sector_size;

	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size));
	zassert_equal(sector_size, sys_le16_to_cpu(boot_sector->BPB_BytsPerSec),
		      "sector size does not match data in FAT root sector (%d vs %d)", sector_size,
		      sys_le16_to_cpu(boot_sector->BPB_BytsPerSec));

	uint32_t sector_count;

	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count));
	zassert_equal(sector_count, sys_le16_to_cpu(boot_sector->BPB_TotSec16),
		      "sector count does not match data in FAT root sector (%d vs %d)",
		      sector_count, sys_le16_to_cpu(boot_sector->BPB_TotSec16));
}

ZTEST(virtual_fat_disk, test_read_multiple_sectors_at_once)
{
	uint8_t sectors[2 * SECTOR_SIZE] = {0};
	const struct fat1x_BS *boot_sector = (const struct fat1x_BS *)sectors;
	const uint8_t *fat = &sectors[SECTOR_SIZE];

	zassert_ok(disk_access_read(DISK_NAME, sectors, 0, 2), "boot+fat multi-sector read failed");

	zassert_equal(sys_le16_to_cpu(boot_sector->BPB_BytsPerSec), SECTOR_SIZE,
		      "unexpected boot sector size");
	zassert_equal(fat[0], boot_sector->BPB_Media, "FAT media byte mismatch");
	zassert_equal(fat[1], 0xFF, "first reserved FAT byte mismatch");
	zassert_equal(fat[2], 0xFF, "second reserved FAT byte mismatch");
}

ZTEST_SUITE(virtual_fat_disk, NULL, virtual_fat_disk_setup, virtual_fat_disk_before, NULL, NULL);
