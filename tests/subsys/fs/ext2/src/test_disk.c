/*
 * Copyright (c) 2026 Tawonga Kondolo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/disk.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/util.h>

#include "test_disk.h"

#if defined(CONFIG_DISK_DRIVER_RAM)

#define BACKING_DISK_NAME "RAM"

static int ext2_test_disk_init(struct disk_info *disk)
{
	ARG_UNUSED(disk);

	return 0;
}

static int ext2_test_disk_status(struct disk_info *disk)
{
	ARG_UNUSED(disk);

	return disk_access_status(BACKING_DISK_NAME);
}

static int ext2_test_disk_read(struct disk_info *disk, uint8_t *data_buf, uint32_t start_sector,
			       uint32_t num_sector)
{
	ARG_UNUSED(disk);

	return disk_access_read(BACKING_DISK_NAME, data_buf, start_sector, num_sector);
}

static int ext2_test_disk_write(struct disk_info *disk, const uint8_t *data_buf,
				uint32_t start_sector, uint32_t num_sector)
{
	ARG_UNUSED(disk);

	return disk_access_write(BACKING_DISK_NAME, data_buf, start_sector, num_sector);
}

static int ext2_test_disk_erase(struct disk_info *disk, uint32_t start_sector, uint32_t num_sector)
{
	ARG_UNUSED(disk);

	return disk_access_erase(BACKING_DISK_NAME, start_sector, num_sector,
				 DISK_ACCESS_ERASE_PHYSICAL);
}

static int ext2_test_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buf)
{
	ARG_UNUSED(disk);

	switch (cmd) {
	case DISK_IOCTL_CTRL_INIT:
	case DISK_IOCTL_CTRL_DEINIT:
		return 0;
	default:
		return disk_access_ioctl(BACKING_DISK_NAME, cmd, buf);
	}
}

static const struct disk_operations ext2_test_disk_ops = {
	.init = ext2_test_disk_init,
	.status = ext2_test_disk_status,
	.read = ext2_test_disk_read,
	.write = ext2_test_disk_write,
	.erase = ext2_test_disk_erase,
	.ioctl = ext2_test_disk_ioctl,
};

static struct disk_info ext2_test_disk = {
	.name = EXT2_TEST_DISK_NAME,
	.ops = &ext2_test_disk_ops,
};

int ext2_test_disk_register(void)
{
	return disk_access_register(&ext2_test_disk);
}

int ext2_test_disk_unregister(void)
{
	return disk_access_unregister(&ext2_test_disk);
}

int ext2_test_disk_reset(void)
{
	int ret;

	while (ext2_test_disk.refcnt > 0U) {
		ret = disk_access_ioctl(EXT2_TEST_DISK_NAME, DISK_IOCTL_CTRL_DEINIT, NULL);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

uint16_t ext2_test_disk_refcount(void)
{
	return ext2_test_disk.refcnt;
}

#endif /* CONFIG_DISK_DRIVER_RAM */
