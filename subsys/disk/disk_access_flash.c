/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <disk/disk_access.h>
#include <errno.h>
#include <init.h>
#include <device.h>
#include <drivers/flash.h>

#define SECTOR_SIZE 512

static struct device *flash_dev;

/* flash read-copy-erase-write operation */
static u8_t read_copy_buf[CONFIG_DISK_ERASE_BLOCK_SIZE];
static u8_t *fs_buff = read_copy_buf;

/* calculate number of blocks required for a given size */
#define GET_NUM_BLOCK(total_size, block_size) \
	((total_size + block_size - 1) / block_size)

#define GET_SIZE_TO_BOUNDARY(start, block_size) \
	(block_size - (start & (block_size - 1)))

static off_t lba_to_address(u32_t sector_num)
{
	off_t flash_addr;

	flash_addr = CONFIG_DISK_FLASH_START + sector_num * SECTOR_SIZE;

	__ASSERT(flash_addr < (CONFIG_DISK_FLASH_START +
		CONFIG_DISK_VOLUME_SIZE), "FS bound error");

	return flash_addr;
}

static int disk_flash_access_status(struct disk_info *disk)
{
	if (!flash_dev) {
		return DISK_STATUS_NOMEDIA;
	}

	return DISK_STATUS_OK;
}

static int disk_flash_access_init(struct disk_info *disk)
{
	if (flash_dev) {
		return 0;
	}

	flash_dev = device_get_binding(CONFIG_DISK_FLASH_DEV_NAME);
	if (!flash_dev) {
		return -ENODEV;
	}

	return 0;
}

static int disk_flash_access_read(struct disk_info *disk, u8_t *buff,
				u32_t start_sector, u32_t sector_count)
{
	off_t fl_addr;
	u32_t remaining;
	u32_t len;
	u32_t num_read;

	fl_addr = lba_to_address(start_sector);
	remaining = (sector_count * SECTOR_SIZE);
	len = CONFIG_DISK_FLASH_MAX_RW_SIZE;

	num_read = GET_NUM_BLOCK(remaining, CONFIG_DISK_FLASH_MAX_RW_SIZE);

	for (u32_t i = 0; i < num_read; i++) {
		if (remaining < CONFIG_DISK_FLASH_MAX_RW_SIZE) {
			len = remaining;
		}

		if (flash_read(flash_dev, fl_addr, buff, len) != 0) {
			return -EIO;
		}

		fl_addr += len;
		buff += len;
		remaining -= len;
	}

	return 0;
}

/* This performs read-copy into an output buffer */
static int read_copy_flash_block(off_t start_addr, u32_t size,
				 const void *src_buff,
				 u8_t *dest_buff)
{
	off_t fl_addr;
	u32_t num_read;
	u32_t offset = 0U;

	/* adjust offset if starting address is not erase-aligned address */
	if (start_addr & (CONFIG_DISK_FLASH_ERASE_ALIGNMENT - 1)) {
		offset = start_addr & (CONFIG_DISK_FLASH_ERASE_ALIGNMENT - 1);
	}

	/* align starting address to an aligned address for flash erase-write */
	fl_addr = ROUND_DOWN(start_addr, CONFIG_DISK_FLASH_ERASE_ALIGNMENT);

	num_read = GET_NUM_BLOCK(CONFIG_DISK_ERASE_BLOCK_SIZE,
				 CONFIG_DISK_FLASH_MAX_RW_SIZE);

	/* read one block from flash */
	for (u32_t i = 0; i < num_read; i++) {
		int rc;

		rc = flash_read(flash_dev,
				fl_addr + (CONFIG_DISK_FLASH_MAX_RW_SIZE * i),
				dest_buff + (CONFIG_DISK_FLASH_MAX_RW_SIZE * i),
				CONFIG_DISK_FLASH_MAX_RW_SIZE);
		if (rc != 0) {
			return -EIO;
		}
	}

	/* overwrite with user data */
	memcpy(dest_buff + offset, src_buff, size);

	return 0;
}

/* input size is either less or equal to a block size,
 * CONFIG_DISK_ERASE_BLOCK_SIZE.
 */
static int update_flash_block(off_t start_addr, u32_t size, const void *buff)
{
	off_t fl_addr;
	u8_t *src = (u8_t *)buff;
	u32_t num_write;

	/* if size is a partial block, perform read-copy with user data */
	if (size < CONFIG_DISK_ERASE_BLOCK_SIZE) {
		int rc;

		rc = read_copy_flash_block(start_addr, size, buff, fs_buff);
		if (rc != 0) {
			return -EIO;
		}

		/* now use the local buffer as the source */
		src = (u8_t *)fs_buff;
	}

	/* always align starting address for flash write operation */
	fl_addr = ROUND_DOWN(start_addr, CONFIG_DISK_FLASH_ERASE_ALIGNMENT);

	/* disable write-protection first before erase */
	flash_write_protection_set(flash_dev, false);
	if (flash_erase(flash_dev, fl_addr, CONFIG_DISK_ERASE_BLOCK_SIZE)
			!= 0) {
		return -EIO;
	}

	/* write data to flash */
	num_write = GET_NUM_BLOCK(CONFIG_DISK_ERASE_BLOCK_SIZE,
				  CONFIG_DISK_FLASH_MAX_RW_SIZE);

	for (u32_t i = 0; i < num_write; i++) {
		/* flash_write reenabled write-protection so disable it again */
		flash_write_protection_set(flash_dev, false);

		if (flash_write(flash_dev, fl_addr, src,
				CONFIG_DISK_FLASH_MAX_RW_SIZE) != 0) {
			return -EIO;
		}

		fl_addr += CONFIG_DISK_FLASH_MAX_RW_SIZE;
		src += CONFIG_DISK_FLASH_MAX_RW_SIZE;
	}

	return 0;
}

static int disk_flash_access_write(struct disk_info *disk, const u8_t *buff,
				 u32_t start_sector, u32_t sector_count)
{
	off_t fl_addr;
	u32_t remaining;
	u32_t size;

	fl_addr = lba_to_address(start_sector);
	remaining = (sector_count * SECTOR_SIZE);

	/* check if start address is erased-aligned address  */
	if (fl_addr & (CONFIG_DISK_FLASH_ERASE_ALIGNMENT - 1)) {
		off_t block_bnd;

		/* not aligned */
		/* check if the size goes over flash block boundary */
		block_bnd = fl_addr + CONFIG_DISK_ERASE_BLOCK_SIZE;
		block_bnd = block_bnd & ~(CONFIG_DISK_ERASE_BLOCK_SIZE - 1);
		if ((fl_addr + remaining) < block_bnd) {
			/* not over block boundary (a partial block also) */
			if (update_flash_block(fl_addr, remaining, buff) != 0) {
				return -EIO;
			}
			return 0;
		}

		/* write goes over block boundary */
		size = GET_SIZE_TO_BOUNDARY(fl_addr,
						CONFIG_DISK_ERASE_BLOCK_SIZE);

		/* write first partial block */
		if (update_flash_block(fl_addr, size, buff) != 0) {
			return -EIO;
		}

		fl_addr += size;
		remaining -= size;
		buff += size;
	}

	/* start is an erase-aligned address */
	while (remaining) {
		int rc;

		if (remaining < CONFIG_DISK_ERASE_BLOCK_SIZE) {
			break;
		}

		rc = update_flash_block(fl_addr, CONFIG_DISK_ERASE_BLOCK_SIZE,
					 buff);
		if (rc != 0) {
			return -EIO;
		}

		fl_addr += CONFIG_DISK_ERASE_BLOCK_SIZE;
		remaining -= CONFIG_DISK_ERASE_BLOCK_SIZE;
		buff += CONFIG_DISK_ERASE_BLOCK_SIZE;
	}

	/* remaining partial block */
	if (remaining) {
		if (update_flash_block(fl_addr, remaining, buff) != 0) {
			return -EIO;
		}
	}

	return 0;
}

static int disk_flash_access_ioctl(struct disk_info *disk, u8_t cmd, void *buff)
{
	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		return 0;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(u32_t *)buff = CONFIG_DISK_VOLUME_SIZE / SECTOR_SIZE;
		return 0;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(u32_t *) buff = SECTOR_SIZE;
		return 0;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ: /* in sectors */
		*(u32_t *)buff = CONFIG_DISK_ERASE_BLOCK_SIZE / SECTOR_SIZE;
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

static struct disk_info flash_disk = {
	.name = CONFIG_DISK_FLASH_VOLUME_NAME,
	.ops = &flash_disk_ops,
};

static int disk_flash_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return disk_access_register(&flash_disk);
}

SYS_INIT(disk_flash_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
