/*
 * Copyright (c) 2023 Endress+Hauser AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_disk_flash

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/disk.h>

LOG_MODULE_REGISTER(disk_flash, CONFIG_FLASH_LOG_LEVEL);

struct disk_flash_data {
	struct k_sem sem;
	uint8_t *const sector_buf;
	uint16_t const sector_buf_size;
	uint32_t disk_total_sector_cnt;
	uint32_t disk_sector_size;
};

struct disk_flash_config {
	uint32_t flash_size;
	struct flash_parameters flash_parameters;
	char *disk_name;
	uint32_t disk_offset;

	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (struct flash_pages_layout fpl;))
};

static void acquire_device(const struct device *dev)
{
	struct disk_flash_data *const data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static void release_device(const struct device *dev)
{
	struct disk_flash_data *const data = dev->data;

	k_sem_give(&data->sem);
}

static int check_boundary(const struct device *dev, off_t offset, size_t len)
{
	int ret = 0;
	const struct disk_flash_config *cfg = dev->config;
	size_t operation_end = offset + len;

	if (cfg->flash_size < operation_end || offset < 0) {
		LOG_ERR("Operation out of bounds");
		ret = -EINVAL;
	}

	return ret;
}

static int disk_flash_read(const struct device *dev, off_t offset, void *dst, size_t len)
{
	int ret = 0;
	uint8_t *dst_buf = dst;
	const struct disk_flash_config *cfg = dev->config;
	struct disk_flash_data *data = dev->data;
	uint32_t write_cursor = 0;

	acquire_device(dev);

	ret = check_boundary(dev, offset, len);
	if (ret != 0) {
		goto release_device;
	}

	if (data->disk_sector_size == 0) {
		ret = -EINVAL;
		goto release_device;
	}

	while (write_cursor < len) {
		uint32_t read_cursor = cfg->disk_offset + offset + write_cursor;
		uint32_t read_sector = read_cursor / data->disk_sector_size;
		uint32_t read_padding = read_cursor % data->disk_sector_size;
		uint32_t len_write = (len - write_cursor);

		if (disk_access_read(cfg->disk_name, data->sector_buf, read_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}

		if (len_write > data->sector_buf_size - read_padding) {
			len_write = data->sector_buf_size - read_padding;
		}

		memcpy(&dst_buf[write_cursor], &data->sector_buf[read_padding], len_write);
		write_cursor += len_write;
	}

release_device:
	release_device(dev);
	return ret;
}

static int disk_flash_write(const struct device *dev, off_t offset, const void *src, size_t len)
{
	int ret = 0;
	const uint8_t *src_buf = src;
	const struct disk_flash_config *cfg = dev->config;
	struct disk_flash_data *data = dev->data;
	uint32_t read_cursor = 0;

	acquire_device(dev);

	ret = check_boundary(dev, offset, len);
	if (ret != 0) {
		goto release_device;
	}

	if (data->disk_sector_size == 0) {
		ret = -EINVAL;
		goto release_device;
	}

	while (read_cursor < len) {
		uint32_t write_cursor = cfg->disk_offset + offset + read_cursor;
		uint32_t write_sector = write_cursor / data->disk_sector_size;
		uint32_t write_padding = write_cursor % data->disk_sector_size;
		uint32_t len_write = (len - read_cursor);

		if (disk_access_read(cfg->disk_name, data->sector_buf, write_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}

		if (len_write > data->sector_buf_size - write_padding) {
			len_write = data->sector_buf_size - write_padding;
		}

		for (size_t i = 0; i < len_write; i++) {
			data->sector_buf[write_padding + i] &= src_buf[read_cursor + i];
		}

		if (disk_access_write(cfg->disk_name, data->sector_buf, write_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}
		read_cursor += len_write;
	}

release_device:
	release_device(dev);
	return ret;
}

static int disk_flash_erase(const struct device *dev, off_t offset, size_t size)
{
	int ret = 0;
	const struct disk_flash_config *cfg = dev->config;
	struct disk_flash_data *data = dev->data;
	uint32_t read_cursor = 0;

	acquire_device(dev);

	ret = check_boundary(dev, offset, size);
	if (ret != 0) {
		goto release_device;
	}

	if (data->disk_sector_size == 0) {
		ret = -EINVAL;
		goto release_device;
	}

	while (read_cursor < size) {
		uint32_t write_cursor = cfg->disk_offset + offset + read_cursor;
		uint32_t write_sector = write_cursor / data->disk_sector_size;
		uint32_t write_padding = write_cursor % data->disk_sector_size;
		uint32_t len_write = (size - read_cursor);

		if (write_sector < 0 || write_sector >= data->disk_total_sector_cnt) {
			ret = -EINVAL;
			goto release_device;
		}

		if (disk_access_read(cfg->disk_name, data->sector_buf, write_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}

		if (len_write > data->sector_buf_size - write_padding) {
			len_write = data->sector_buf_size - write_padding;
		}

		memset(&data->sector_buf[write_padding],
			cfg->flash_parameters.erase_value,
			len_write);

		if (disk_access_write(cfg->disk_name, data->sector_buf, write_sector, 1)) {
			ret = -EINVAL;
			goto release_device;
		}
		read_cursor += len_write;
	}

release_device:
	release_device(dev);
	return ret;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void disk_flash_pages_layout(const struct device *dev,
				    const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct disk_flash_config *cfg = dev->config;

	*layout = &cfg->fpl;
	*layout_size = 1;
}
#endif

static const struct flash_parameters *disk_flash_get_parameters(const struct device *dev)
{
	const struct disk_flash_config *cfg = dev->config;

	return &cfg->flash_parameters;
}

static const struct flash_driver_api disk_flash_api = {
	.read = disk_flash_read,
	.write = disk_flash_write,
	.erase = disk_flash_erase,
	.get_parameters = disk_flash_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = disk_flash_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = NULL,
	.read_jedec_id = NULL,
#endif
#if defined(CONFIG_FLASH_EX_OP_ENABLED)
	.ex_op = NULL,
#endif
};

static int disk_flash_init(const struct device *dev)
{
	int res = 0;
	struct disk_flash_data *data = dev->data;
	const struct disk_flash_config *cfg = dev->config;
	uint64_t disk_size = 0;

	k_sem_init(&data->sem, 1, 1);

	res = disk_access_init(cfg->disk_name);
	if (res != 0) {
		LOG_ERR("init disk failed: %d", res);
		goto init_done;
	}

	res = disk_access_ioctl(cfg->disk_name, DISK_IOCTL_GET_SECTOR_COUNT,
				&data->disk_total_sector_cnt);
	if (res != 0) {
		LOG_ERR("read total sector cnt failed: %d", res);
		goto init_done;
	}


	res = disk_access_ioctl(cfg->disk_name, DISK_IOCTL_GET_SECTOR_SIZE,
				&data->disk_sector_size);
	if (res != 0) {
		LOG_ERR("read sector size failed: %d", res);
		goto init_done;
	}

	if (data->disk_sector_size > data->sector_buf_size) {
		LOG_ERR("sector size %u of disk to big for buffer %u", data->disk_sector_size,
			(uint32_t)data->sector_buf_size);
		res = -EINVAL;
		goto init_done;
	}

	if (cfg->disk_offset % data->disk_sector_size != 0) {
		LOG_WRN("offset is not aligned with disk sectors (disk sector size: %u)",
			data->disk_sector_size);
	}

	disk_size = data->disk_sector_size * data->disk_total_sector_cnt;
	if ((disk_size - cfg->disk_offset) < cfg->flash_size) {
		LOG_ERR("Underlying disk to small to support flash: %lu < %d",
			(unsigned long)(disk_size - cfg->disk_offset),
			cfg->flash_size);
		res = -EINVAL;
		goto init_done;
	}

init_done:
	return res;
}

#define DISK_FLASH_DEFINE(index)								\
	BUILD_ASSERT(DT_INST_PROP(index, size) > 0, "flash_size needs to be > 0");		\
	BUILD_ASSERT(DT_INST_PROP(index, page_size) > 0, "page_size needs to be > 0");		\
	BUILD_ASSERT(DT_INST_PROP(index, disk_offset) >= 0, "disk_offset needs to be >= 0");	\
	uint8_t sector_buf_##index[DT_INST_PROP(index, disk_sector_size)];			\
	static struct disk_flash_data disk_flash_data_##index = {				\
		.sector_buf = sector_buf_##index,						\
		.sector_buf_size = sizeof(sector_buf_##index),					\
	};											\
	static const struct disk_flash_config disk_flash_config_##index = {			\
		.flash_size = DT_INST_PROP(index, size) / 8,					\
		.flash_parameters =								\
		{										\
			.write_block_size =							\
				COND_CODE_0(DT_INST_PROP(index, write_block_size),		\
					(DT_INST_PROP(index, disk_sector_size)),		\
					(DT_INST_PROP(index, write_block_size))),		\
			.erase_value = 0xFF							\
		},										\
		IF_ENABLED(									\
			CONFIG_FLASH_PAGE_LAYOUT,						\
			(.fpl =									\
				{								\
					.pages_count =						\
						(DT_INST_PROP(index, size) / 8)			\
						/ DT_INST_PROP(index, page_size),		\
					.pages_size = DT_INST_PROP(index, page_size)		\
				},								\
			)									\
		)										\
		.disk_name = DT_INST_PROP(index, disk_name),					\
		.disk_offset = DT_INST_PROP(index, disk_offset),				\
	};											\
	DEVICE_DT_INST_DEFINE(index, &disk_flash_init, NULL, &disk_flash_data_##index,		\
			&disk_flash_config_##index,						\
			DT_STRING_TOKEN(DT_DRV_INST(index), init_level),			\
			CONFIG_FLASH_INIT_PRIORITY, &disk_flash_api);

DT_INST_FOREACH_STATUS_OKAY(DISK_FLASH_DEFINE)
