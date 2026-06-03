/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/memc.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(memc_ramdisk, CONFIG_MEMC_RAMDISK_LOG_LEVEL);

struct memc_ramdisk_config {
	const struct device *memc_dev;
	size_t sector_size;
	size_t sector_count;
	const char *disk_name;
};

struct memc_ramdisk_data {
	struct disk_info info;
};

static int memc_ramdisk_init_cb(struct disk_info *disk)
{
	ARG_UNUSED(disk);
	return 0;
}

static int memc_ramdisk_status(struct disk_info *disk)
{
	ARG_UNUSED(disk);
	return DISK_STATUS_OK;
}

static int memc_ramdisk_read(struct disk_info *disk, uint8_t *buf, uint32_t sector, uint32_t count)
{
	const struct device *dev = disk->dev;
	const struct memc_ramdisk_config *cfg = dev->config;
	uint32_t last = sector + count;

	if (last < sector || last > cfg->sector_count) {
		LOG_ERR("%s: read out of range: sector %u + %u > %zu", cfg->disk_name, sector,
			count, cfg->sector_count);
		return -EINVAL;
	}

	return memc_read(cfg->memc_dev, (uint32_t)(sector * cfg->sector_size), buf,
			 count * cfg->sector_size);
}

static int memc_ramdisk_write(struct disk_info *disk, const uint8_t *buf, uint32_t sector,
			      uint32_t count)
{
	const struct device *dev = disk->dev;
	const struct memc_ramdisk_config *cfg = dev->config;
	uint32_t last = sector + count;

	if (last < sector || last > cfg->sector_count) {
		LOG_ERR("%s: write out of range: sector %u + %u > %zu", cfg->disk_name, sector,
			count, cfg->sector_count);
		return -EINVAL;
	}

	return memc_write(cfg->memc_dev, (uint32_t)(sector * cfg->sector_size), buf,
			  count * cfg->sector_size);
}

static int memc_ramdisk_erase(struct disk_info *disk, uint32_t sector, uint32_t count)
{
	const struct device *dev = disk->dev;
	const struct memc_ramdisk_config *cfg = dev->config;
	uint32_t last = sector + count;
	uint8_t zeros[64];
	int rc;

	if (last < sector || last > cfg->sector_count) {
		return -EINVAL;
	}

	/* No hardware erase in MEMC API; zero-fill for consistent post-erase reads. */
	memset(zeros, 0, sizeof(zeros));

	size_t total = (size_t)count * cfg->sector_size;
	uint32_t offset = (uint32_t)(sector * cfg->sector_size);

	while (total > 0) {
		size_t chunk = MIN(total, sizeof(zeros));

		rc = memc_write(cfg->memc_dev, offset, zeros, chunk);
		if (rc) {
			return rc;
		}
		offset += chunk;
		total -= chunk;
	}
	return 0;
}

static int memc_ramdisk_ioctl(struct disk_info *disk, uint8_t cmd, void *buf)
{
	const struct device *dev = disk->dev;
	const struct memc_ramdisk_config *cfg = dev->config;

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
	case DISK_IOCTL_CTRL_INIT:
	case DISK_IOCTL_CTRL_DEINIT:
		return 0;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buf = (uint32_t)cfg->sector_count;
		return 0;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buf = (uint32_t)cfg->sector_size;
		return 0;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buf = 1U;
		return 0;
	default:
		return -EINVAL;
	}
}

static const struct disk_operations memc_ramdisk_ops = {
	.init = memc_ramdisk_init_cb,
	.status = memc_ramdisk_status,
	.read = memc_ramdisk_read,
	.write = memc_ramdisk_write,
	.erase = memc_ramdisk_erase,
	.ioctl = memc_ramdisk_ioctl,
};

static int memc_ramdisk_init(const struct device *dev)
{
	const struct memc_ramdisk_config *cfg = dev->config;
	struct memc_ramdisk_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->memc_dev)) {
		LOG_ERR("%s: MEMC device %s not ready", cfg->disk_name, cfg->memc_dev->name);
		return -ENODEV;
	}

	data->info.dev = dev;
	ret = disk_access_register(&data->info);
	if (ret < 0) {
		LOG_ERR("%s: disk_access_register failed: %d", cfg->disk_name, ret);
		return ret;
	}

	return 0;
}

#define MEMC_RAMDISK_DEFINE(n)                                                                     \
	static const struct memc_ramdisk_config memc_ramdisk_cfg_##n = {                           \
		.memc_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, memc)),                               \
		.sector_size = DT_INST_PROP(n, sector_size),                                       \
		.sector_count = DT_INST_PROP(n, sector_count),                                     \
		.disk_name = DT_INST_PROP(n, disk_name),                                           \
	};                                                                                         \
	static struct memc_ramdisk_data memc_ramdisk_data_##n = {                                  \
		.info =                                                                            \
			{                                                                          \
				.name = DT_INST_PROP(n, disk_name),                                \
				.ops = &memc_ramdisk_ops,                                          \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, memc_ramdisk_init, NULL, &memc_ramdisk_data_##n,                  \
			      &memc_ramdisk_cfg_##n, POST_KERNEL,                                  \
			      CONFIG_MEMC_RAMDISK_INIT_PRIORITY, NULL);

#define DT_DRV_COMPAT zephyr_memc_ram_disk

DT_INST_FOREACH_STATUS_OKAY(MEMC_RAMDISK_DEFINE)
