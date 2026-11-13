/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/disk/virtual_fat_disk.h>
#include <zephyr/fs/virtual_fat.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT zephyr_virtual_fat_disk

LOG_MODULE_REGISTER(virtual_fat_disk, CONFIG_VIRTUAL_FAT_DISK_LOG_LEVEL);

struct virtual_fat_disk_config {
	size_t sector_cnt;
};

struct virtual_fat_disk_data {
	struct disk_info info;
	struct virtual_fat_ctx *fat_ctx;
	struct k_mutex lock;
};

int disk_register_virtual_files(const struct device *dev, const struct virtual_file *files,
				size_t files_len)
{
	struct virtual_fat_disk_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	const int ret = register_virtual_files(data->fat_ctx, files, files_len);

	k_mutex_unlock(&data->lock);

	return ret;
}

void disk_register_new_file_write_cb(const struct device *dev, put_file_content_chunk_t callback,
				     void *user_data)
{
	struct virtual_fat_disk_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	register_new_file_write_cb(data->fat_ctx, callback, user_data);
	k_mutex_unlock(&data->lock);
}

void disk_set_volume_label(const struct device *dev, const char *label)
{
	struct virtual_fat_disk_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	set_volume_label(data->fat_ctx, label);
	k_mutex_unlock(&data->lock);
}

static size_t get_sector_size(const struct device *dev)
{
	struct virtual_fat_disk_data *data = dev->data;

	return data->fat_ctx->sector_size;
}

static int disk_virtual_fat_init(struct disk_info *disk)
{
	LOG_DBG("");
	return 0;
}

static int disk_virtual_fat_status(struct disk_info *disk)
{
	LOG_DBG("");
	return DISK_STATUS_OK;
}

static int disk_virtual_fat_read(struct disk_info *disk, uint8_t *data_buf, uint32_t start_sector,
				 uint32_t num_sector)
{
	LOG_DBG("");

	struct virtual_fat_disk_data *data = disk->dev->data;

	for (uint32_t i = 0; i < num_sector; ++i) {
		k_mutex_lock(&data->lock, K_FOREVER);
		int ret = virtual_fat_read(data->fat_ctx, data_buf + i * get_sector_size(disk->dev),
					   get_sector_size(disk->dev), start_sector + i, 0);

		if (ret != 0) {
			k_mutex_unlock(&data->lock);
			LOG_ERR("Failed to read sector %d", start_sector + i);
			return ret;
		}
		k_mutex_unlock(&data->lock);
	}

	return 0;
}

static int disk_virtual_fat_write(struct disk_info *disk, const uint8_t *data_buf,
				  uint32_t start_sector, uint32_t num_sector)
{
	LOG_DBG("");

	struct virtual_fat_disk_data *data = disk->dev->data;

	for (uint32_t i = 0; i < num_sector; ++i) {
		k_mutex_lock(&data->lock, K_FOREVER);
		int ret =
			virtual_fat_write(data->fat_ctx, data_buf + i * get_sector_size(disk->dev),
					  get_sector_size(disk->dev), start_sector + i, 0);

		if (ret != 0) {
			k_mutex_unlock(&data->lock);
			LOG_ERR("Failed to write sector %d", start_sector + i);
			return ret;
		}
		k_mutex_unlock(&data->lock);
	}

	return 0;
}

static int disk_virtual_fat_erase(struct disk_info *disk, uint32_t start_sector,
				  uint32_t num_sector)
{
	LOG_DBG("");
	return 0;
}

static int disk_virtual_fat_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	const struct virtual_fat_disk_config *cfg = disk->dev->config;

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		LOG_DBG("DISK_IOCTL_GET_SECTOR_COUNT");
		*(uint32_t *)buff = cfg->sector_cnt;
		break;

	case DISK_IOCTL_GET_SECTOR_SIZE:
		LOG_DBG("DISK_IOCTL_GET_SECTOR_SIZE");
		*(uint32_t *)buff = get_sector_size(disk->dev);
		break;

	case DISK_IOCTL_RESERVED:
		LOG_DBG("DISK_IOCTL_RESERVED");
		break;

	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		LOG_DBG("DISK_IOCTL_GET_ERASE_BLOCK_SZ");
		*(uint32_t *)buff = 1;
		break;

	case DISK_IOCTL_CTRL_SYNC:
		LOG_DBG("DISK_IOCTL_CTRL_SYNC");
		break;

	case DISK_IOCTL_CTRL_INIT:
		LOG_DBG("DISK_IOCTL_CTRL_INIT");
		break;

	case DISK_IOCTL_CTRL_DEINIT:
		LOG_DBG("DISK_IOCTL_CTRL_DEINIT");
		break;

	case DISK_IOCTL_GET_CARD_CID:
		LOG_DBG("DISK_IOCTL_GET_CARD_CID");
		return -ENOTSUP;

	default:
		LOG_ERR("Unknown ioctl command %d", cmd);
		return -EINVAL;
	}

	return 0;
}

static const struct disk_operations virtual_fat_disk_ops = {
	.init = disk_virtual_fat_init,
	.status = disk_virtual_fat_status,
	.read = disk_virtual_fat_read,
	.write = disk_virtual_fat_write,
	.erase = disk_virtual_fat_erase,
	.ioctl = disk_virtual_fat_ioctl,
};

static int virtual_fat_disk_init(const struct device *dev)
{
	struct virtual_fat_disk_data *data = dev->data;

	k_mutex_init(&data->lock);

	const int rc = disk_access_register(&data->info);

	if (rc < 0) {
		LOG_ERR("Failed to register disk %s error %d", data->info.name, rc);
		return rc;
	}

	return 0;
}

#define VIRTUAL_FAT_DISK_INST_DEFINE(n)                                                            \
	VIRTUAL_FAT_DEFINE_PARTITION(virtual_fat_disk_fat_partition_##n,                           \
				     DT_INST_PROP(n, disk_name), (DT_INST_PROP(n, sector_size)),   \
				     (DT_INST_PROP(n, data_partition_size)));                      \
                                                                                                   \
	static const struct virtual_fat_disk_config virtual_fat_disk_config_##n = {                \
		.sector_cnt = FAT_BS_TOT_SEC(DT_INST_PROP(n, sector_size),                         \
					     DT_INST_PROP(n, data_partition_size))};               \
                                                                                                   \
	static struct virtual_fat_disk_data virtual_fat_disk_data_##n = {                          \
		.info = {.ops = &virtual_fat_disk_ops,                                             \
			 .name = DT_INST_PROP(n, disk_name),                                       \
			 .dev = DEVICE_DT_INST_GET(n)},                                            \
		.fat_ctx = &virtual_fat_disk_fat_partition_##n,                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, virtual_fat_disk_init, NULL, &virtual_fat_disk_data_##n,          \
			      &virtual_fat_disk_config_##n, POST_KERNEL,                           \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &virtual_fat_disk_ops);

DT_INST_FOREACH_STATUS_OKAY(VIRTUAL_FAT_DISK_INST_DEFINE)
