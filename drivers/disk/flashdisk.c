/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <drivers/disk.h>
#include <errno.h>
#include <init.h>
#include <device.h>
#include <drivers/flash.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(flashdisk, CONFIG_FLASHDISK_LOG_LEVEL);

struct mtd_disk_data {
	struct disk_info info;
	off_t offset;
	size_t size;
	size_t write_bs;
	size_t page_size;
};

#define SECTOR_SIZE CONFIG_DISK_FLASH_SECTOR_SIZE

/*
 * FIXME: The buffer must be large enough, there are no checks for it.
 * Also there is no mutual exclusion for this buffer.
 */
static uint8_t __aligned(4) read_copy_buf[CONFIG_DISK_ERASE_BLOCK_SIZE];
/* flash read-copy-erase-write operation */
static uint8_t *fs_buff = read_copy_buf;

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
	struct mtd_disk_data *ctx = NULL;
	int rc;
	struct flash_pages_info page;

	if (disk) {
		ctx = CONTAINER_OF(disk, struct mtd_disk_data, info);
	}

	if (disk == NULL || ctx == NULL) {
		LOG_ERR("Failed to obtain context pointer");
		return -ENODEV;
	}

	if (!device_is_ready(disk->dev)) {
		LOG_ERR("Device is not ready");
		return -ENODEV;
	}

	rc = flash_get_page_info_by_offs(disk->dev, ctx->offset, &page);
	if (rc != 0) {
		LOG_ERR("Error %d while getting page info", rc);
		return rc;
	}

	ctx->write_bs = flash_get_write_block_size(disk->dev);
	ctx->page_size = page.size;
	LOG_INF("Initialize device %s", disk->name);
	LOG_INF("offset %lx, write bs %zu, page size %zu, volume size %zu",
		(long)ctx->offset, ctx->write_bs, ctx->page_size, ctx->size);

	return 0;
}

static int disk_flash_access_read(struct disk_info *disk, uint8_t *buff,
				uint32_t start_sector, uint32_t sector_count)
{
	struct mtd_disk_data *ctx;
	off_t fl_addr;
	uint32_t remaining;
	uint32_t len;
	uint32_t num_read;

	ctx = CONTAINER_OF(disk, struct mtd_disk_data, info);

	fl_addr = ctx->offset + start_sector * SECTOR_SIZE;
	if (fl_addr >= (ctx->offset + ctx->size)) {
		LOG_ERR("Address %lx outside partition boundary", (long)fl_addr);
		return -EINVAL;
	}

	remaining = (sector_count * SECTOR_SIZE);
	len = ctx->page_size;
	num_read = GET_NUM_BLOCK(remaining, ctx->page_size);

	for (uint32_t i = 0; i < num_read; i++) {
		if (remaining < ctx->page_size) {
			len = remaining;
		}

		if (flash_read(disk->dev, fl_addr, buff, len) != 0) {
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
	struct mtd_disk_data *ctx;
	off_t fl_addr;
	uint32_t num_read;
	uint32_t offset = 0U;

	ctx = CONTAINER_OF(disk, struct mtd_disk_data, info);

	/* adjust offset if starting address is not erase-aligned address */
	if (start_addr & (ctx->page_size - 1)) {
		offset = start_addr & (ctx->page_size - 1);
	}

	/* align starting address to an aligned address for flash erase-write */
	fl_addr = ROUND_DOWN(start_addr, ctx->page_size);

	num_read = GET_NUM_BLOCK(ctx->page_size, ctx->page_size);

	/* read one block from flash */
	for (uint32_t i = 0; i < num_read; i++) {
		int rc;

		rc = flash_read(disk->dev,
				fl_addr + (ctx->page_size * i),
				dest_buff + (ctx->page_size * i),
				ctx->page_size);
		if (rc != 0) {
			return -EIO;
		}
	}

	/* overwrite with user data */
	memcpy(dest_buff + offset, src_buff, size);

	return 0;
}

/* input size is either less or equal to a block size,
 * ctx->page_size.
 */
static int update_flash_block(struct disk_info *disk, off_t start_addr,
			      uint32_t size, const void *buff)
{
	struct mtd_disk_data *ctx;
	off_t fl_addr;
	uint8_t *src = (uint8_t *)buff;
	uint32_t num_write;

	ctx = CONTAINER_OF(disk, struct mtd_disk_data, info);

	/* if size is a partial block, perform read-copy with user data */
	if (size < ctx->page_size) {
		int rc;

		rc = read_copy_flash_block(disk, start_addr, size, buff, fs_buff);
		if (rc != 0) {
			return -EIO;
		}

		/* now use the local buffer as the source */
		src = (uint8_t *)fs_buff;
	}

	/* always align starting address for flash write operation */
	fl_addr = ROUND_DOWN(start_addr, ctx->page_size);

	/* disable write-protection first before erase */
	flash_write_protection_set(disk->dev, false);
	if (flash_erase(disk->dev, fl_addr, ctx->page_size) != 0) {
		return -EIO;
	}

	/* write data to flash */
	num_write = GET_NUM_BLOCK(ctx->page_size, ctx->page_size);

	for (uint32_t i = 0; i < num_write; i++) {
		/* flash_write reenabled write-protection so disable it again */
		flash_write_protection_set(disk->dev, false);

		if (flash_write(disk->dev, fl_addr, src, ctx->page_size) != 0) {
			return -EIO;
		}

		fl_addr += ctx->page_size;
		src += ctx->page_size;
	}

	return 0;
}

static int disk_flash_access_write(struct disk_info *disk, const uint8_t *buff,
				 uint32_t start_sector, uint32_t sector_count)
{
	struct mtd_disk_data *ctx;
	off_t fl_addr;
	uint32_t remaining;
	uint32_t size;

	ctx = CONTAINER_OF(disk, struct mtd_disk_data, info);

	fl_addr = ctx->offset + start_sector * SECTOR_SIZE;
	if (fl_addr >= (ctx->offset + ctx->size)) {
		LOG_ERR("Address %lx outside partition boundary", (long)fl_addr);
		return -EINVAL;
	}

	remaining = (sector_count * SECTOR_SIZE);

	/* check if start address is erased-aligned address  */
	if (fl_addr & (ctx->page_size - 1)) {
		off_t block_bnd;

		/* not aligned */
		/* check if the size goes over flash block boundary */
		block_bnd = fl_addr + ctx->page_size;
		block_bnd = block_bnd & ~(ctx->page_size - 1);
		if ((fl_addr + remaining) < block_bnd) {
			/* not over block boundary (a partial block also) */
			if (update_flash_block(disk, fl_addr, remaining, buff) != 0) {
				return -EIO;
			}
			return 0;
		}

		/* write goes over block boundary */
		size = GET_SIZE_TO_BOUNDARY(fl_addr, ctx->page_size);

		/* write first partial block */
		if (update_flash_block(disk, fl_addr, size, buff) != 0) {
			return -EIO;
		}

		fl_addr += size;
		remaining -= size;
		buff += size;
	}

	/* start is an erase-aligned address */
	while (remaining) {
		int rc;

		if (remaining < ctx->page_size) {
			break;
		}

		rc = update_flash_block(disk, fl_addr, ctx->page_size, buff);
		if (rc != 0) {
			return -EIO;
		}

		fl_addr += ctx->page_size;
		remaining -= ctx->page_size;
		buff += ctx->page_size;
	}

	/* remaining partial block */
	if (remaining) {
		if (update_flash_block(disk, fl_addr, remaining, buff) != 0) {
			return -EIO;
		}
	}

	return 0;
}

static int disk_flash_access_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	struct mtd_disk_data *ctx;

	ctx = CONTAINER_OF(disk, struct mtd_disk_data, info);

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		return 0;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buff = ctx->size / SECTOR_SIZE;
		return 0;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *) buff = SECTOR_SIZE;
		return 0;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ: /* in sectors */
		*(uint32_t *)buff = ctx->page_size / SECTOR_SIZE;
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

#define DT_DRV_COMPAT fixed_partitions

#define DEFINE_FLASHDISKS_MTD(node_id)						\
{										\
	.info = {								\
		.name = DT_LABEL(node_id),					\
		.ops = &flash_disk_ops,						\
		.dev = DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(node_id)),	\
	},									\
	.offset = DT_REG_ADDR(node_id),						\
	.size = DT_REG_SIZE(node_id),						\
},

#define DEFINE_FLASHDISKS(inst)					\
	DT_INST_FOREACH_CHILD(inst, DEFINE_FLASHDISKS_MTD)

static struct mtd_disk_data mtd_disks[] = {
	DT_INST_FOREACH_STATUS_OKAY(DEFINE_FLASHDISKS)
};

static void disk_flash_fix_part_name(struct disk_info *disk)
{
	char part_label[] = "storage";
	static char *part_name = "NAND";

	if (strncmp(disk->name, part_label, sizeof(part_label)) == 0) {
		disk->name = part_name;
	}
}

static int disk_flash_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int rc = 0;

	for (int i = 0; i < ARRAY_SIZE(mtd_disks); i++) {
		disk_flash_fix_part_name(&mtd_disks[i].info);
		rc |= disk_access_register(&mtd_disks[i].info);
	}

	return rc;
}

SYS_INIT(disk_flash_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
