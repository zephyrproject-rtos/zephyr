/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/disk.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flashdisk, CONFIG_FLASHDISK_LOG_LEVEL);

struct flashdisk_data {
	struct disk_info info;
	const unsigned int area_id;
	const off_t offset;
	const size_t size;
	const size_t sector_size;
	size_t page_size;
};

#define DT_DRV_COMPAT zephyr_flash_disk

#define DEFINE_BUFFER(n) uint8_t _buf##n[DT_INST_PROP(n, cache_size)];
#define MAX_BUF_SIZE sizeof(union{DT_INST_FOREACH_STATUS_OKAY(DEFINE_BUFFER)})

/* flash read-copy-erase-write operation */
static uint8_t __aligned(4) read_copy_buf[MAX_BUF_SIZE];
static K_MUTEX_DEFINE(read_copy_buf_lock);

/* calculate number of blocks required for a given size */
#define GET_NUM_BLOCK(total_size, block_size) \
	((total_size + block_size - 1) / block_size)

#define GET_SIZE_TO_BOUNDARY(start, block_size) \
	(block_size - (start & (block_size - 1)))

static int disk_flash_access_status(struct disk_info *disk)
{
	LOG_DBG("status : %s", disk->dev ? "okay" : "no media");
	if (!disk->dev) {
		return DISK_STATUS_NOMEDIA;
	}

	return DISK_STATUS_OK;
}

static int disk_flash_access_init(struct disk_info *disk)
{
	struct flashdisk_data *ctx;
	const struct flash_area *fap;
	int rc;
	struct flash_pages_info page;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	rc = flash_area_open(ctx->area_id, &fap);
	if (rc < 0) {
		LOG_ERR("Flash area %u open error %d", ctx->area_id, rc);
		return rc;
	}

	disk->dev = flash_area_get_device(fap);

	rc = flash_get_page_info_by_offs(disk->dev, ctx->offset, &page);
	if (rc < 0) {
		LOG_ERR("Error %d while getting page info", rc);
		flash_area_close(fap);
		return rc;
	}

	ctx->page_size = page.size;
	LOG_INF("Initialize device %s", disk->name);
	LOG_INF("offset %lx, sector size %zu, page size %zu, volume size %zu",
		(long)ctx->offset, ctx->sector_size, ctx->page_size, ctx->size);

	if (ctx->page_size > ARRAY_SIZE(read_copy_buf)) {
		LOG_ERR("Buffer too small (%zu needs %zu)",
			ARRAY_SIZE(read_copy_buf), ctx->page_size);
		flash_area_close(fap);
		return -ENOMEM;
	}

	return 0;
}

static bool sectors_in_range(struct flashdisk_data *ctx,
			     uint32_t start_sector, uint32_t sector_count)
{
	uint32_t start, end;

	start = ctx->offset + (start_sector * ctx->sector_size);
	end = start + (sector_count * ctx->sector_size);

	if ((end >= start) && (start >= ctx->offset) && (end <= ctx->offset + ctx->size)) {
		return true;
	}

	LOG_ERR("sector start %" PRIu32 " count %" PRIu32
		" outside partition boundary", start_sector, sector_count);
	return false;
}

static int disk_flash_access_read(struct disk_info *disk, uint8_t *buff,
				uint32_t start_sector, uint32_t sector_count)
{
	struct flashdisk_data *ctx;
	off_t fl_addr;
	uint32_t remaining;
	uint32_t len;
	uint32_t num_read;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	if (!sectors_in_range(ctx, start_sector, sector_count)) {
		return -EINVAL;
	}

	fl_addr = ctx->offset + start_sector * ctx->sector_size;
	remaining = (sector_count * ctx->sector_size);
	len = ctx->page_size;
	num_read = GET_NUM_BLOCK(remaining, ctx->page_size);

	for (uint32_t i = 0; i < num_read; i++) {
		if (remaining < ctx->page_size) {
			len = remaining;
		}

		if (flash_read(disk->dev, fl_addr, buff, len) < 0) {
			return -EIO;
		}

		fl_addr += len;
		buff += len;
		remaining -= len;
	}

	return 0;
}

/* This performs read-copy into an output buffer */
static int read_copy_flash_block(struct disk_info *disk,
				 off_t start_addr,
				 uint32_t size,
				 const void *src_buff,
				 uint8_t *dest_buff)
{
	struct flashdisk_data *ctx;
	off_t fl_addr;
	uint32_t offset;
	uint32_t remaining;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	/* adjust offset if starting address is not erase-aligned address */
	offset = start_addr & (ctx->page_size - 1);

	/* align starting address to page boundary */
	fl_addr = ROUND_DOWN(start_addr, ctx->page_size);

	if (offset > 0) {
		/* read page contents prior to user data */
		if (flash_read(disk->dev, fl_addr, dest_buff, offset) < 0) {
			return -EIO;
		}
	}

	/* write user data in page buffer */
	memcpy(dest_buff + offset, src_buff, size);

	remaining = ctx->page_size - (offset + size);
	if (remaining) {
		/* read page contents after user data */
		if (flash_read(disk->dev, start_addr + size,
				&dest_buff[offset + size], remaining) < 0) {
			return -EIO;
		}
	}

	return 0;
}

static int overwrite_flash_block(struct disk_info *disk, off_t fl_addr,
				 const void *buff)
{
	struct flashdisk_data *ctx;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	if (flash_erase(disk->dev, fl_addr, ctx->page_size) < 0) {
		return -EIO;
	}

	/* write data to flash */
	if (flash_write(disk->dev, fl_addr, buff, ctx->page_size) < 0) {
		return -EIO;
	}

	return 0;
}

/* input size is either less or equal to a block size (ctx->page_size)
 * and write data never spans across adjacent blocks.
 */
static int update_flash_block(struct disk_info *disk, off_t start_addr,
			      uint32_t size, const void *buff)
{
	struct flashdisk_data *ctx;
	off_t fl_addr;
	int rc;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	/* always align starting address for flash write operation */
	fl_addr = ROUND_DOWN(start_addr, ctx->page_size);

	/* when writing full page the address must be page aligned
	 * when writing partial page user data must be within a single page
	 */
	__ASSERT_NO_MSG(fl_addr + ctx->page_size >= start_addr + size);

	if (size == ctx->page_size) {
		rc = overwrite_flash_block(disk, fl_addr, buff);
	} else {
		/* partial block, perform read-copy with user data */
		k_mutex_lock(&read_copy_buf_lock, K_FOREVER);
		rc = read_copy_flash_block(disk, start_addr, size, buff, read_copy_buf);
		if (rc == 0) {
			rc = overwrite_flash_block(disk, fl_addr, read_copy_buf);
		}
		k_mutex_unlock(&read_copy_buf_lock);
	}

	return rc == 0 ? 0 : -EIO;
}

static int disk_flash_access_write(struct disk_info *disk, const uint8_t *buff,
				 uint32_t start_sector, uint32_t sector_count)
{
	struct flashdisk_data *ctx;
	off_t fl_addr;
	uint32_t remaining;
	uint32_t size;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	if (!sectors_in_range(ctx, start_sector, sector_count)) {
		return -EINVAL;
	}

	fl_addr = ctx->offset + start_sector * ctx->sector_size;
	remaining = (sector_count * ctx->sector_size);

	/* check if start address is erased-aligned address  */
	if (fl_addr & (ctx->page_size - 1)) {
		off_t block_bnd;

		/* not aligned */
		/* check if the size goes over flash block boundary */
		block_bnd = fl_addr + ctx->page_size;
		block_bnd = block_bnd & ~(ctx->page_size - 1);
		if ((fl_addr + remaining) <= block_bnd) {
			/* not over block boundary (a partial block also) */
			if (update_flash_block(disk, fl_addr, remaining, buff) < 0) {
				return -EIO;
			}
			return 0;
		}

		/* write goes over block boundary */
		size = GET_SIZE_TO_BOUNDARY(fl_addr, ctx->page_size);

		/* write first partial block */
		if (update_flash_block(disk, fl_addr, size, buff) < 0) {
			return -EIO;
		}

		fl_addr += size;
		remaining -= size;
		buff += size;
	}

	/* start is an erase-aligned address */
	while (remaining) {
		if (remaining < ctx->page_size) {
			break;
		}

		if (update_flash_block(disk, fl_addr, ctx->page_size, buff) < 0) {
			return -EIO;
		}

		fl_addr += ctx->page_size;
		remaining -= ctx->page_size;
		buff += ctx->page_size;
	}

	/* remaining partial block */
	if (remaining) {
		if (update_flash_block(disk, fl_addr, remaining, buff) < 0) {
			return -EIO;
		}
	}

	return 0;
}

static int disk_flash_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct flashdisk_data *ctx;

	ctx = CONTAINER_OF(disk, struct flashdisk_data, info);

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		return 0;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buff = ctx->size / ctx->sector_size;
		return 0;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buff = ctx->sector_size;
		return 0;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ: /* in sectors */
		*(uint32_t *)buff = ctx->page_size / ctx->sector_size;
		return 0;
	default:
		break;
	}

	return -EINVAL;
}

static const struct disk_operations flash_disk_ops = {
	.init = disk_flash_access_init,
	.status = disk_flash_access_status,
	.read = disk_flash_access_read,
	.write = disk_flash_access_write,
	.ioctl = disk_flash_access_ioctl,
};

#define PARTITION_PHANDLE(n) DT_PHANDLE_BY_IDX(DT_DRV_INST(n), partition, 0)

#define DEFINE_FLASHDISKS_DEVICE(n)						\
{										\
	.info = {								\
		.ops = &flash_disk_ops,						\
		.name = DT_INST_PROP(n, disk_name),				\
	},									\
	.area_id = DT_FIXED_PARTITION_ID(PARTITION_PHANDLE(n)),			\
	.offset = DT_REG_ADDR(PARTITION_PHANDLE(n)),				\
	.size = DT_REG_SIZE(PARTITION_PHANDLE(n)),				\
	.sector_size = DT_INST_PROP(n, sector_size),				\
},

static struct flashdisk_data flash_disks[] = {
	DT_INST_FOREACH_STATUS_OKAY(DEFINE_FLASHDISKS_DEVICE)
};

#define VERIFY_CACHE_SIZE_IS_MULTIPLY_OF_SECTOR_SIZE(n)					\
	BUILD_ASSERT(DT_INST_PROP(n, cache_size) % DT_INST_PROP(n, sector_size) == 0,	\
		"Devicetree node " DT_NODE_PATH(DT_DRV_INST(n))				\
		" has cache size which is not a multiple of its sector size");
DT_INST_FOREACH_STATUS_OKAY(VERIFY_CACHE_SIZE_IS_MULTIPLY_OF_SECTOR_SIZE)

static int disk_flash_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int err = 0;

	for (int i = 0; i < ARRAY_SIZE(flash_disks); i++) {
		int rc;

		rc = disk_access_register(&flash_disks[i].info);
		if (rc < 0) {
			LOG_ERR("Failed to register disk %s error %d",
				flash_disks[i].info.name, rc);
			err = rc;
		}
	}

	return err;
}

SYS_INIT(disk_flash_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
