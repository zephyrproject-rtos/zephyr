/*
 * Copyright (c) 2023-2024 Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/unistd.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_interface.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/loopback_disk.h>

LOG_MODULE_REGISTER(loopback_disk_access, CONFIG_LOOPBACK_DISK_LOG_LEVEL);

#define LOOPBACK_SECTOR_SIZE CONFIG_LOOPBACK_DISK_SECTOR_SIZE

static inline struct loopback_disk_access *get_ctx(struct disk_info *info)
{
	return CONTAINER_OF(info, struct loopback_disk_access, info);
}

static int loopback_disk_access_init(struct disk_info *disk)
{
	return 0;
}
static int loopback_disk_access_status(struct disk_info *disk)
{
	return DISK_STATUS_OK;
}
static int loopback_disk_access_read(struct disk_info *disk, uint8_t *data_buf,
				     uint32_t start_sector, uint32_t num_sector)
{
	struct loopback_disk_access *ctx = get_ctx(disk);

	int ret = fs_seek(&ctx->file, start_sector * LOOPBACK_SECTOR_SIZE, FS_SEEK_SET);

	if (ret != 0) {
		LOG_ERR("Failed to seek backing file: %d", ret);
		return ret;
	}

	const size_t total_len = num_sector * LOOPBACK_SECTOR_SIZE;
	size_t len_left = total_len;

	while (len_left > 0) {
		ret = fs_read(&ctx->file, data_buf, len_left);
		if (ret < 0) {
			LOG_ERR("Failed to read from backing file: %d", ret);
			return ret;
		}
		if (ret == 0) {
			LOG_WRN("Tried to read past end of backing file");
			return -EIO;
		}
		__ASSERT(ret <= len_left,
			 "fs_read returned more than we asked for: %d instead of %ld", ret,
			 len_left);
		len_left -= ret;
	}

	return 0;
}
static int loopback_disk_access_write(struct disk_info *disk, const uint8_t *data_buf,
				      uint32_t start_sector, uint32_t num_sector)
{
	struct loopback_disk_access *ctx = get_ctx(disk);

	if (start_sector + num_sector > ctx->num_sectors) {
		LOG_WRN("Tried to write past end of backing file");
		return -EIO;
	}

	int ret = fs_seek(&ctx->file, start_sector * LOOPBACK_SECTOR_SIZE, FS_SEEK_SET);

	if (ret != 0) {
		LOG_ERR("Failed to seek backing file: %d", ret);
		return ret;
	}

	const size_t total_len = num_sector * LOOPBACK_SECTOR_SIZE;
	size_t buf_offset = 0;

	while (buf_offset < total_len) {
		ret = fs_write(&ctx->file, &data_buf[buf_offset], total_len - buf_offset);
		if (ret < 0) {
			LOG_ERR("Failed to write to backing file: %d", ret);
			return ret;
		}
		if (ret == 0) {
			LOG_ERR("0-byte write to backing file");
			return -EIO;
		}
		buf_offset += ret;
	}

	return 0;
}
static int loopback_disk_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct loopback_disk_access *ctx = get_ctx(disk);

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT: {
		*(uint32_t *)buff = ctx->num_sectors;
		return 0;
	}
	case DISK_IOCTL_GET_SECTOR_SIZE: {
		*(uint32_t *)buff = LOOPBACK_SECTOR_SIZE;
		return 0;
	}
	case DISK_IOCTL_CTRL_SYNC:
		return fs_sync(&ctx->file);
	default:
		return -ENOTSUP;
	}
}

static const struct disk_operations loopback_disk_operations = {
	.init = loopback_disk_access_init,
	.status = loopback_disk_access_status,
	.read = loopback_disk_access_read,
	.write = loopback_disk_access_write,
	.ioctl = loopback_disk_access_ioctl,
};

int loopback_disk_access_register(struct loopback_disk_access *ctx, const char *file_path,
				  const char *disk_access_name)
{
	ctx->file_path = file_path;

	ctx->info.name = disk_access_name;
	ctx->info.ops = &loopback_disk_operations;

	struct fs_dirent entry;
	int ret = fs_stat(ctx->file_path, &entry);

	if (ret != 0) {
		LOG_ERR("Failed to stat backing file: %d", ret);
		return ret;
	}
	if (entry.size % LOOPBACK_SECTOR_SIZE != 0) {
		LOG_WRN("Backing file is not a multiple of sector size (%d bytes), "
			"rounding down: %ld bytes",
			LOOPBACK_SECTOR_SIZE, entry.size);
	}
	ctx->num_sectors = entry.size / LOOPBACK_SECTOR_SIZE;

	fs_file_t_init(&ctx->file);
	ret = fs_open(&ctx->file, ctx->file_path, FS_O_READ | FS_O_WRITE);
	if (ret != 0) {
		LOG_ERR("Failed to open backing file: %d", ret);
		return ret;
	}

	ret = disk_access_register(&ctx->info);
	if (ret != 0) {
		LOG_ERR("Failed to register disk access: %d", ret);
		return ret;
	}

	return 0;
}

int loopback_disk_access_unregister(struct loopback_disk_access *ctx)
{
	int ret;

	ret = disk_access_unregister(&ctx->info);
	if (ret != 0) {
		LOG_ERR("Failed to unregister disk access: %d", ret);
		return ret;
	}
	ctx->info.name = NULL;
	ctx->info.ops = NULL;

	ret = fs_close(&ctx->file);
	if (ret != 0) {
		LOG_ERR("Failed to close backing file: %d", ret);
		return ret;
	}

	return 0;
}
