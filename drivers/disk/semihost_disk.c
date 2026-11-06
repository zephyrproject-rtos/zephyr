/*
 * Copyright (c) 2026 RBR Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_semihost_disk

#include <zephyr/arch/common/semihost.h>
#include <zephyr/device.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(semihost_disk, CONFIG_SEMIHOST_DISK_LOG_LEVEL);

#define ERASE_FILL_BUFFER_SIZE 256

static const uint8_t zero_fill[ERASE_FILL_BUFFER_SIZE];

struct semihost_disk_config {
	const char *filename;
	uint32_t sector_size;
	uint32_t sector_count;
};

struct semihost_disk_data {
	struct disk_info info;
	struct k_mutex lock;
	long fd;
};

static bool semihost_disk_in_range(const struct semihost_disk_config *config, uint32_t sector,
				   uint32_t count)
{
	uint32_t last_sector = sector + count;

	if (last_sector < sector || last_sector > config->sector_count) {
		LOG_ERR("Sector %" PRIu32 " is outside the range %" PRIu32, last_sector,
			config->sector_count);
		return false;
	}

	return true;
}

static int semihost_disk_access_status(struct disk_info *disk)
{
	return DISK_STATUS_OK;
}

static int semihost_disk_access_read(struct disk_info *disk, uint8_t *buff, uint32_t sector,
				     uint32_t count)
{
	const struct semihost_disk_config *config = disk->dev->config;
	struct semihost_disk_data *data = disk->dev->data;
	long len;
	int ret = 0;

	if (!semihost_disk_in_range(config, sector, count)) {
		return -EIO;
	}

	if (count == 0) {
		/* Semihost read of zero bytes reports end of file */
		return 0;
	}

	len = (long)count * config->sector_size;

	k_mutex_lock(&data->lock, K_FOREVER);
	if (semihost_seek(data->fd, (long)sector * config->sector_size) != 0 ||
	    semihost_read(data->fd, buff, len) != len) {
		ret = -EIO;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int semihost_disk_access_write(struct disk_info *disk, const uint8_t *buff, uint32_t sector,
				      uint32_t count)
{
	const struct semihost_disk_config *config = disk->dev->config;
	struct semihost_disk_data *data = disk->dev->data;
	long len;
	int ret = 0;

	if (!semihost_disk_in_range(config, sector, count)) {
		return -EIO;
	}

	if (count == 0) {
		return 0;
	}

	len = (long)count * config->sector_size;

	k_mutex_lock(&data->lock, K_FOREVER);
	if (semihost_seek(data->fd, (long)sector * config->sector_size) != 0 ||
	    semihost_write(data->fd, buff, len) != 0) {
		ret = -EIO;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int semihost_disk_access_erase(struct disk_info *disk, uint32_t sector, uint32_t count)
{
	const struct semihost_disk_config *config = disk->dev->config;
	struct semihost_disk_data *data = disk->dev->data;
	long remaining;
	int ret = 0;

	if (!semihost_disk_in_range(config, sector, count)) {
		return -EINVAL;
	}

	remaining = (long)count * config->sector_size;

	k_mutex_lock(&data->lock, K_FOREVER);
	if (semihost_seek(data->fd, (long)sector * config->sector_size) != 0) {
		ret = -EIO;
	}
	while (ret == 0 && remaining > 0) {
		long chunk = MIN(remaining, (long)sizeof(zero_fill));

		if (semihost_write(data->fd, zero_fill, chunk) != 0) {
			ret = -EIO;
		}
		remaining -= chunk;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int semihost_disk_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	const struct semihost_disk_config *config = disk->dev->config;

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
		*(uint32_t *)buff = 1U;
		break;
	case DISK_IOCTL_CTRL_INIT:
	case DISK_IOCTL_CTRL_DEINIT:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int semihost_disk_access_init(struct disk_info *disk)
{
	return semihost_disk_access_ioctl(disk, DISK_IOCTL_CTRL_INIT, NULL);
}

static const struct disk_operations semihost_disk_ops = {
	.init = semihost_disk_access_init,
	.status = semihost_disk_access_status,
	.read = semihost_disk_access_read,
	.write = semihost_disk_access_write,
	.erase = semihost_disk_access_erase,
	.ioctl = semihost_disk_access_ioctl,
};

static int semihost_disk_init(const struct device *dev)
{
	const struct semihost_disk_config *config = dev->config;
	struct semihost_disk_data *data = dev->data;
	long size = (long)config->sector_count * config->sector_size;
	long flen;
	int ret;

	k_mutex_init(&data->lock);

	data->fd = semihost_open(config->filename, SEMIHOST_OPEN_RB_PLUS);
	if (data->fd < 0) {
		/* File does not exist, create it */
		data->fd = semihost_open(config->filename, SEMIHOST_OPEN_WB_PLUS);
		if (data->fd < 0) {
			LOG_ERR("Cannot open host file %s", config->filename);
			return -ENODEV;
		}
	}

	flen = semihost_flen(data->fd);
	if (flen < 0) {
		LOG_ERR("Cannot get the size of host file %s", config->filename);
		semihost_close(data->fd);
		return -EIO;
	}

	/* Extend the file to the disk size, so reads never hit its end */
	if (flen < size && (semihost_seek(data->fd, size - 1) != 0 ||
			    semihost_write(data->fd, zero_fill, 1) != 0)) {
		LOG_ERR("Cannot extend host file %s", config->filename);
		semihost_close(data->fd);
		return -EIO;
	}

	data->info.dev = dev;

	ret = disk_access_register(&data->info);
	if (ret != 0) {
		semihost_close(data->fd);
	}

	return ret;
}

#define SEMIHOST_DISK_DEFINE(n)                                                                    \
	BUILD_ASSERT((uint64_t)DT_INST_PROP(n, sector_count) * DT_INST_PROP(n, sector_size) <=     \
			     LONG_MAX,                                                             \
		     "Disk size exceeds the semihosting file offset range");                       \
                                                                                                   \
	static const struct semihost_disk_config semihost_disk_config_##n = {                      \
		.filename = DT_INST_PROP(n, filename),                                             \
		.sector_size = DT_INST_PROP(n, sector_size),                                       \
		.sector_count = DT_INST_PROP(n, sector_count),                                     \
	};                                                                                         \
                                                                                                   \
	static struct semihost_disk_data semihost_disk_data_##n = {                                \
		.info.name = DT_INST_PROP(n, disk_name),                                           \
		.info.ops = &semihost_disk_ops,                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, semihost_disk_init, NULL, &semihost_disk_data_##n,                \
			      &semihost_disk_config_##n, POST_KERNEL,                              \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &semihost_disk_ops);

DT_INST_FOREACH_STATUS_OKAY(SEMIHOST_DISK_DEFINE)
