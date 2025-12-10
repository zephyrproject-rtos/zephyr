/*
 * Copyright (c) 2025 Endress+Hauser GmbH+Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_ftl_dhara

#include <dhara/map.h>
#include <dhara/nand.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/math_extras.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(disk_ftl_dhara, CONFIG_DISK_FTL_LOG_LEVEL);

struct disk_ftl_data {
	struct k_sem lock;
	bool initialised;
	struct disk_info info;
	const struct flash_area *area;
	off_t partition_offset;
	size_t page_size;
	size_t block_size;
	size_t partition_size;
	size_t buffer_size;
	uint8_t *page_buffer;
	struct dhara_map dhara_map;
	struct dhara_nand dhara_nand;
	uint8_t *dhara_buffer;
	uint8_t dhara_gc_ratio;
};

int dhara_nand_is_bad(const struct dhara_nand *n, dhara_block_t b)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(n, struct disk_ftl_data, dhara_nand);
	int block_status = FLASH_BLOCK_BAD;
	int ret;

	ret = flash_ex_op(ctx->info.dev, FLASH_IS_BAD_BLOCK, (uintptr_t)&b, &block_status);
	if (ret == -ENOTSUP) {
		LOG_DBG("Checking bad block is not supported");
		return 0;
	} else if (ret != 0) {
		LOG_ERR("Checking bad block at 0x%X failed with error %d", b * ctx->block_size,
			ret);
		return 1;
	} else if (block_status == FLASH_BLOCK_BAD) {
		LOG_INF("Block at 0x%X is marked bad", b * ctx->block_size);
	} else {
		/* No more action if block is good */
	}

	return (block_status == FLASH_BLOCK_BAD) ? 1 : 0;
}

void dhara_nand_mark_bad(const struct dhara_nand *n, dhara_block_t b)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(n, struct disk_ftl_data, dhara_nand);
	int ret;

	ret = flash_ex_op(ctx->info.dev, FLASH_MARK_BAD_BLOCK, (uintptr_t)&b, NULL);
	if (ret == -ENOTSUP) {
		LOG_INF("Marking bad block is not supported");
	} else if (ret != 0) {
		LOG_ERR("Marking bad block failed with error %d", ret);
	} else {
		LOG_DBG("Marked block bad at 0x%X", b * ctx->block_size);
	}
}

int dhara_nand_erase(const struct dhara_nand *n, dhara_block_t b, dhara_error_t *err)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(n, struct disk_ftl_data, dhara_nand);
	int ret;

	LOG_DBG("%s: block at 0x%X", __func__, b * ctx->block_size);

	ret = flash_erase(ctx->info.dev, b * ctx->block_size, ctx->block_size);
	if (ret != 0) {
		LOG_ERR("Erasing block at 0x%X failed with error %d", b * ctx->block_size, ret);
		*err = DHARA_E_BAD_BLOCK;
		return -1;
	}

	return 0;
}

int dhara_nand_prog(const struct dhara_nand *n, dhara_page_t p, const uint8_t *data,
		    dhara_error_t *err)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(n, struct disk_ftl_data, dhara_nand);
	int ret;

	LOG_DBG("%s: page at 0x%X", __func__, p * ctx->page_size);

	ret = flash_write(ctx->info.dev, p * ctx->page_size, data, ctx->page_size);
	if (ret != 0) {
		LOG_ERR("Writing to page at 0x%X failed with error %d", p * ctx->page_size, ret);
		*err = DHARA_E_BAD_BLOCK;
		return -1;
	}

	return 0;
}

int dhara_nand_is_free(const struct dhara_nand *n, dhara_page_t p)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(n, struct disk_ftl_data, dhara_nand);
	const struct flash_parameters *flash_params = flash_get_parameters(ctx->info.dev);
	uint8_t empty_page[ctx->page_size];
	int ret;

	LOG_DBG("%s: page at 0x%X", __func__, p * ctx->page_size);

	ret = flash_read(ctx->info.dev, p * ctx->page_size, ctx->page_buffer, ctx->page_size);
	if (ret != 0) {
		LOG_ERR("Reading page at 0x%X failed with error %d", p * ctx->page_size, ret);
		return 0;
	}

	memset(empty_page, flash_params->erase_value, sizeof(empty_page));
	if (memcmp(ctx->page_buffer, empty_page, ctx->page_size) != 0) {
		return 0;
	}

	return 1;
}

int dhara_nand_read(const struct dhara_nand *n, dhara_page_t p, size_t offset, size_t length,
		    uint8_t *data, dhara_error_t *err)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(n, struct disk_ftl_data, dhara_nand);
	int ret;

	LOG_DBG("%s: page at 0x%X", __func__, p * ctx->page_size);

	ret = flash_read(ctx->info.dev, p * ctx->page_size + offset, data, length);
	if (ret != 0) {
		LOG_ERR("Reading data at 0x%X failed with error %d", p * ctx->page_size + offset,
			ret);
		*err = DHARA_E_ECC;
		return -1;
	}

	return 0;
}

int dhara_nand_copy(const struct dhara_nand *n, dhara_page_t src, dhara_page_t dst,
		    dhara_error_t *err)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(n, struct disk_ftl_data, dhara_nand);
	int ret;

	LOG_DBG("%s: 0x%X -> 0x%X", __func__, src * ctx->page_size, dst * ctx->page_size);

	ret = flash_read(ctx->info.dev, src * ctx->page_size, ctx->page_buffer, ctx->page_size);
	if (ret != 0) {
		LOG_ERR("Reading page at 0x%X failed with error %d", src * ctx->page_size, ret);
		*err = DHARA_E_ECC;
		return -1;
	}

	ret = flash_write(ctx->info.dev, dst * ctx->page_size, ctx->page_buffer, ctx->page_size);
	if (ret != 0) {
		LOG_ERR("Writing to page at 0x%X failed with error %d", dst * ctx->page_size, ret);
		*err = DHARA_E_BAD_BLOCK;
		return -1;
	}

	return 0;
}

int disk_ftl_access_init(struct disk_info *disk)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(disk, struct disk_ftl_data, info);
	const struct flash_parameters *flash_params;
	struct flash_pages_info page; /* Information of the smallest erasable area */
	dhara_error_t err;
	off_t offset;
	int ret;

	k_sem_init(&ctx->lock, 1, 1);
	k_sem_take(&ctx->lock, K_FOREVER);

	if (ctx->initialised) {
		LOG_ERR("FTL is already initialised");
		k_sem_give(&ctx->lock);
		return -EALREADY;
	}

	if (!flash_area_device_is_ready(ctx->area)) {
		LOG_ERR("Flash device %s is not ready", ctx->area->fa_dev->name);
		k_sem_give(&ctx->lock);
		return -ENODEV;
	}

	ctx->info.dev = flash_area_get_device(ctx->area);

	if (ctx->info.dev == NULL) {
		LOG_ERR("Flash device was not found");
		k_sem_give(&ctx->lock);
		return -ENODEV;
	}

	flash_params = flash_get_parameters(disk->dev);
	ctx->page_size = flash_params->write_block_size;
	if (ctx->page_size > ctx->buffer_size) {
		LOG_ERR("Buffer size %u is too small for pages with size %u", ctx->buffer_size,
			ctx->page_size);
		k_sem_give(&ctx->lock);
		return -ENOMEM;
	}

	ret = flash_get_page_info_by_offs(disk->dev, ctx->partition_offset, &page);
	if (ret != 0) {
		LOG_ERR("Getting flash page info at 0x%lX failed with error %d",
			ctx->partition_offset, ret);
		k_sem_give(&ctx->lock);
		return ret;
	}

	if (ctx->partition_offset != page.start_offset) {
		LOG_ERR("Partition does not start at beginning of an erase block");
		k_sem_give(&ctx->lock);
		return -EINVAL;
	}

	ctx->block_size = page.size;
	offset = ctx->partition_offset + page.size;

	while (offset < ctx->partition_offset + ctx->partition_size) {
		ret = flash_get_page_info_by_offs(disk->dev, offset, &page);
		if (ret != 0) {
			LOG_ERR("Getting flash page info at 0x%lX failed with error %d", offset,
				ret);
			k_sem_give(&ctx->lock);
			return ret;
		}

		if (page.size != ctx->block_size) {
			LOG_ERR("Non-uniform block size is not supported");
			k_sem_give(&ctx->lock);
			return -EINVAL;
		}

		offset += page.size;
	}

	if (offset != ctx->partition_offset + ctx->partition_size) {
		LOG_ERR("Last block does not end at partition boundary");
		k_sem_give(&ctx->lock);
		return -EINVAL;
	}

	ctx->dhara_nand.log2_page_size = LOG2CEIL(ctx->page_size);
	ctx->dhara_nand.log2_ppb = LOG2CEIL(ctx->block_size / ctx->page_size);
	ctx->dhara_nand.num_blocks = ctx->partition_size / ctx->block_size;

	LOG_DBG("Initialise Dhara with log2_page_size=%u, log2_ppb=%u, num_blocks=%u",
		ctx->dhara_nand.log2_page_size, ctx->dhara_nand.log2_ppb,
		ctx->dhara_nand.num_blocks);

	dhara_map_init(&ctx->dhara_map, &ctx->dhara_nand, ctx->dhara_buffer, ctx->dhara_gc_ratio);

	ret = dhara_map_resume(&ctx->dhara_map, &err);
	if (ret != 0) {
		LOG_INF("dhara_map_resume failed with error %d", err);
	}

	ctx->initialised = true;
	k_sem_give(&ctx->lock);

	return 0;
}

static int disk_ftl_access_status(struct disk_info *disk)
{
	if (!disk->dev) {
		return DISK_STATUS_NOMEDIA;
	}

	if (!device_is_ready(disk->dev)) {
		return DISK_STATUS_UNINIT;
	}

	return DISK_STATUS_OK;
}

static int disk_ftl_access_read(struct disk_info *disk, uint8_t *data_buf, uint32_t start_sector,
				uint32_t num_sector)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(disk, struct disk_ftl_data, info);
	uint32_t end_sector;
	dhara_error_t err;
	int ret;

	if (u32_add_overflow(start_sector, num_sector, &end_sector) ||
	    (end_sector > (ctx->partition_size / ctx->page_size))) {
		LOG_ERR("Requested sectors are out of range");
		return -EINVAL;
	}

	k_sem_take(&ctx->lock, K_FOREVER);

	for (uint32_t i = 0; i < num_sector; i++) {
		uint32_t sector = start_sector + i;
		uint8_t *buffer = data_buf + (i * ctx->page_size);

		ret = dhara_map_read(&ctx->dhara_map, sector, buffer, &err);
		if (ret == 0) {
			continue;
		}

		LOG_ERR("dhara_map_read failed with error %d", err);
		if (err != DHARA_E_ECC) {
			k_sem_give(&ctx->lock);
			return -EIO;
		}

		ret = dhara_map_write(&ctx->dhara_map, sector, buffer, &err);
		if (ret != 0) {
			LOG_ERR("dhara_map_write failed with error %d", err);
			k_sem_give(&ctx->lock);
			return -EIO;
		}
	}

	k_sem_give(&ctx->lock);

	return 0;
}

static int disk_ftl_access_write(struct disk_info *disk, const uint8_t *data_buf,
				 uint32_t start_sector, uint32_t num_sector)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(disk, struct disk_ftl_data, info);
	uint32_t end_sector;
	dhara_error_t err;
	int ret;

	if (u32_add_overflow(start_sector, num_sector, &end_sector) ||
	    (end_sector > (ctx->partition_size / ctx->page_size))) {
		LOG_ERR("Requested sectors are out of range");
		return -EINVAL;
	}

	k_sem_take(&ctx->lock, K_FOREVER);

	for (uint32_t i = 0; i < num_sector; i++) {
		uint32_t sector = start_sector + i;
		const uint8_t *buffer = data_buf + (i * ctx->page_size);

		ret = dhara_map_write(&ctx->dhara_map, sector, buffer, &err);
		if (ret != 0) {
			LOG_ERR("dhara_map_write failed with error %d", err);
			k_sem_give(&ctx->lock);
			return -EIO;
		}
	}

	k_sem_give(&ctx->lock);

	return 0;
}

static int disk_ftl_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct disk_ftl_data *ctx = CONTAINER_OF(disk, struct disk_ftl_data, info);
	dhara_error_t err;
	int ret;

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		k_sem_take(&ctx->lock, K_FOREVER);
		*(uint32_t *)buff = dhara_map_capacity(&ctx->dhara_map);
		k_sem_give(&ctx->lock);
		break;

	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buff = ctx->page_size;
		break;

	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buff = ctx->block_size;
		break;

	case DISK_IOCTL_CTRL_SYNC:
	case DISK_IOCTL_CTRL_DEINIT:
		k_sem_take(&ctx->lock, K_FOREVER);
		ret = dhara_map_sync(&ctx->dhara_map, &err);
		k_sem_give(&ctx->lock);
		if (ret != 0) {
			LOG_ERR("dhara_map_sync failed with error %d", err);
			return -EIO;
		}
		break;

	case DISK_IOCTL_CTRL_INIT:
		return disk_ftl_access_init(disk);

	default:
		LOG_ERR("Unsupported ioctl command %u", cmd);
		return -ENOTSUP;
	}

	return 0;
}

static int disk_ftl_init(const struct device *dev)
{
	struct disk_ftl_data *dev_data = dev->data;

	return disk_access_register(&dev_data->info);
}

static const struct disk_operations disk_ftl_ops = {
	.init = disk_ftl_access_init,
	.status = disk_ftl_access_status,
	.read = disk_ftl_access_read,
	.write = disk_ftl_access_write,
	.ioctl = disk_ftl_access_ioctl,
};

#define PARTITION_PHANDLE(n) DT_PHANDLE(DT_DRV_INST(n), partition)

#define DISK_FTL_INIT(n)                                                                           \
	static uint8_t disk_ftl_page_buffer_##n[DT_INST_PROP(n, buffer_size)];                     \
                                                                                                   \
	static uint8_t disk_ftl_dhara_buffer_##n[DT_INST_PROP(n, buffer_size)];                    \
                                                                                                   \
	static struct disk_ftl_data disk_ftl_data_##n = {                                          \
		.info =                                                                            \
			{                                                                          \
				.name = DT_INST_PROP(n, disk_name),                                \
				.ops = &disk_ftl_ops,                                              \
			},                                                                         \
		.area = FIXED_PARTITION_BY_NODE(PARTITION_PHANDLE(n)),                             \
		.partition_offset = FIXED_PARTITION_NODE_OFFSET(PARTITION_PHANDLE(n)),             \
		.partition_size = FIXED_PARTITION_NODE_SIZE(PARTITION_PHANDLE(n)),                 \
		.buffer_size = DT_INST_PROP(n, buffer_size),                                       \
		.page_buffer = disk_ftl_page_buffer_##n,                                           \
		.dhara_buffer = disk_ftl_dhara_buffer_##n,                                         \
		.dhara_gc_ratio = DT_INST_PROP(n, gc_ratio),                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, disk_ftl_init, NULL, &disk_ftl_data_##n, NULL, POST_KERNEL,       \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &disk_ftl_ops);

DT_INST_FOREACH_STATUS_OKAY(DISK_FTL_INIT)
