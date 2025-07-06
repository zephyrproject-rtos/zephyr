/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2021,2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/drivers/disk.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ramdisk, CONFIG_RAMDISK_LOG_LEVEL);

struct ram_disk_data {
	struct disk_info info;
	const size_t sector_size;
	const size_t sector_count;
	uint8_t *const buf;
};

struct ram_disk_config {
	const size_t sector_size;
	const size_t sector_count;
	const size_t size;
	uint8_t *const buf;
};

static void *lba_to_address(const struct device *dev, uint32_t lba)
{
	const struct ram_disk_config *config = dev->config;

	return &config->buf[lba * config->sector_size];
}

static int disk_ram_access_status(struct disk_info *disk)
{
	return DISK_STATUS_OK;
}

static int disk_ram_access_read(struct disk_info *disk, uint8_t *buff,
				uint32_t sector, uint32_t count)
{
	const struct device *dev = disk->dev;
	const struct ram_disk_config *config = dev->config;
	uint32_t last_sector = sector + count;

	if (last_sector < sector || last_sector > config->sector_count) {
		LOG_ERR("Sector %" PRIu32 " is outside the range %zu",
			last_sector, config->sector_count);
		return -EIO;
	}

	memcpy(buff, lba_to_address(dev, sector), count * config->sector_size);

	return 0;
}

static int disk_ram_access_write(struct disk_info *disk, const uint8_t *buff,
				 uint32_t sector, uint32_t count)
{
	const struct device *dev = disk->dev;
	const struct ram_disk_config *config = dev->config;
	uint32_t last_sector = sector + count;

	if (last_sector < sector || last_sector > config->sector_count) {
		LOG_ERR("Sector %" PRIu32 " is outside the range %zu",
			last_sector, config->sector_count);
		return -EIO;
	}

	memcpy(lba_to_address(dev, sector), buff, count * config->sector_size);

	return 0;
}

static int disk_ram_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	const struct ram_disk_config *config = disk->dev->config;

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buff = config->sector_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buff = config->sector_size;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buff  = 1U;
		break;
	case DISK_IOCTL_CTRL_INIT:
	case DISK_IOCTL_CTRL_DEINIT:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int disk_ram_access_init(struct disk_info *disk)
{
	return disk_ram_access_ioctl(disk, DISK_IOCTL_CTRL_INIT, NULL);
}

static int disk_ram_init(const struct device *dev)
{
	struct disk_info *info = dev->data;

	info->dev = dev;

	return disk_access_register(info);
}

static const struct disk_operations ram_disk_ops = {
	.init = disk_ram_access_init,
	.status = disk_ram_access_status,
	.read = disk_ram_access_read,
	.write = disk_ram_access_write,
	.ioctl = disk_ram_access_ioctl,
};

#define DT_DRV_COMPAT zephyr_ram_disk

#define RAMDISK_DEVICE_SIZE(n)								\
	(DT_INST_PROP(n, sector_size) * DT_INST_PROP(n, sector_count))

#define RAMDISK_DEVICE_CONFIG_DEFINE_MEMREG(n)						\
	BUILD_ASSERT(RAMDISK_DEVICE_SIZE(n) <=						\
		     DT_REG_SIZE(DT_INST_PHANDLE(n, ram_region)),			\
		     "Disk size is smaller than memory region");			\
											\
	static struct ram_disk_config disk_config_##n = {				\
		.sector_size = DT_INST_PROP(n, sector_size),				\
		.sector_count = DT_INST_PROP(n, sector_count),				\
		.size = RAMDISK_DEVICE_SIZE(n),						\
		.buf = UINT_TO_POINTER(DT_REG_ADDR(DT_INST_PHANDLE(n, ram_region))),	\
	}

#define RAMDISK_DEVICE_CONFIG_DEFINE_LOCAL(n)						\
	static uint8_t disk_buf_##n[DT_INST_PROP(n, sector_size) *			\
				    DT_INST_PROP(n, sector_count)];			\
											\
	static struct ram_disk_config disk_config_##n = {				\
		.sector_size = DT_INST_PROP(n, sector_size),				\
		.sector_count = DT_INST_PROP(n, sector_count),				\
		.size = RAMDISK_DEVICE_SIZE(n),						\
		.buf = disk_buf_##n,							\
	}

#define RAMDISK_DEVICE_CONFIG_DEFINE(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, ram_region),				\
		    (RAMDISK_DEVICE_CONFIG_DEFINE_MEMREG(n)),				\
		    (RAMDISK_DEVICE_CONFIG_DEFINE_LOCAL(n)))

#define RAMDISK_DEVICE_DEFINE(n)							\
											\
	static struct disk_info disk_info_##n  = {					\
		.name = DT_INST_PROP(n, disk_name),					\
		.ops = &ram_disk_ops,							\
	};										\
											\
	RAMDISK_DEVICE_CONFIG_DEFINE(n);						\
											\
	DEVICE_DT_INST_DEFINE(n, disk_ram_init, NULL,					\
			      &disk_info_##n, &disk_config_##n,				\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &ram_disk_ops);

DT_INST_FOREACH_STATUS_OKAY(RAMDISK_DEVICE_DEFINE)
