/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdint.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <diskio.h>
#include <ff.h>
#include <device.h>
#include <flash.h>

static struct device *flash_dev;

/* flash read-copy-erase-write operation */
static uint8_t read_copy_buf[CONFIG_FS_BLOCK_SIZE];
static uint8_t *fs_buff = read_copy_buf;

/* calculate number of blocks required for a given size */
#define GET_NUM_BLOCK(total_size, block_size) \
	((total_size + block_size - 1) / block_size)

#define GET_SIZE_TO_BOUNDARY(start, block_size) \
	(block_size - (start & (block_size - 1)))

static off_t lba_to_address(uint32_t sector_num)
{
	off_t flash_addr;

	flash_addr = CONFIG_FS_FLASH_START + sector_num * _MIN_SS;
	__ASSERT(flash_addr < (CONFIG_FS_FLASH_START + CONFIG_FS_VOLUME_SIZE),
		 "FS bound error");
	return flash_addr;
}

DSTATUS fat_disk_status(void)
{
	if (!flash_dev) {
		return STA_NOINIT;
	}

	return RES_OK;
}

DSTATUS fat_disk_initialize(void)
{
	if (flash_dev) {
		return RES_OK;
	}

	flash_dev = device_get_binding(CONFIG_FS_FLASH_DEV_NAME);
	if (!flash_dev) {
		return STA_NOINIT;
	}

	return RES_OK;
}

DRESULT fat_disk_read(void *buff, uint32_t start_sector,
		      uint32_t sector_count)
{
	off_t fl_addr;
	uint32_t remaining;
	uint32_t len;
	uint32_t num_read;

	fl_addr = lba_to_address(start_sector);
	remaining = (sector_count * _MIN_SS);
	len = CONFIG_FS_FLASH_MAX_RW_SIZE;

	num_read = GET_NUM_BLOCK(remaining, CONFIG_FS_FLASH_MAX_RW_SIZE);

	for (uint32_t i = 0; i < num_read; i++) {
		if (remaining < CONFIG_FS_FLASH_MAX_RW_SIZE) {
			len = remaining;
		}

		if (flash_read(flash_dev, fl_addr, buff, len) != 0) {
			return RES_ERROR;
		}

		fl_addr += len;
		buff += len;
		remaining -= len;
	}

	return RES_OK;
}

/* This performs read-copy into an output buffer */
static DRESULT read_copy_flash_block(off_t start_addr, uint32_t size,
				     const void *src_buff,
				     uint8_t *dest_buff)
{
	off_t fl_addr;
	uint32_t num_read;
	uint32_t offset = 0;

	/* adjust offset if starting address is not erase-aligned address */
	if (start_addr & (CONFIG_FS_FLASH_ERASE_ALIGNMENT - 1)) {
		offset = start_addr & (CONFIG_FS_FLASH_ERASE_ALIGNMENT - 1);
	}

	/* align starting address to an aligned address for flash erase-write */
	fl_addr = ROUND_DOWN(start_addr, CONFIG_FS_FLASH_ERASE_ALIGNMENT);

	num_read = GET_NUM_BLOCK(CONFIG_FS_BLOCK_SIZE,
				 CONFIG_FS_FLASH_MAX_RW_SIZE);

	/* read one block from flash */
	for (uint32_t i = 0; i < num_read; i++) {
		if (flash_read(flash_dev,
			fl_addr + (CONFIG_FS_FLASH_MAX_RW_SIZE * i),
			dest_buff + (CONFIG_FS_FLASH_MAX_RW_SIZE * i),
			CONFIG_FS_FLASH_MAX_RW_SIZE) != 0) {
			return RES_ERROR;
		}
	}

	/* overwrite with user data */
	memcpy(dest_buff + offset, src_buff, size);

	return RES_OK;
}

/* input size is either less or equal to a block size, CONFIG_FS_BLOCK_SIZE. */
static DRESULT update_flash_block(off_t start_addr, uint32_t size,
				  const void *buff)
{
	off_t fl_addr;
	uint8_t *src = (uint8_t *)buff;
	uint32_t num_write;

	/* if size is a partial block, perform read-copy with user data */
	if (size < CONFIG_FS_BLOCK_SIZE) {
		if (read_copy_flash_block(start_addr, size, buff, fs_buff) !=
		    RES_OK) {
			return RES_ERROR;
		}

		/* now use the local buffer as the source */
		src = (uint8_t *)fs_buff;
	}

	/* always align starting address for flash write operation */
	fl_addr = ROUND_DOWN(start_addr, CONFIG_FS_FLASH_ERASE_ALIGNMENT);

	/* disable write-protection first before erase */
	flash_write_protection_set(flash_dev, false);
	if (flash_erase(flash_dev, fl_addr, CONFIG_FS_BLOCK_SIZE) != 0) {
		return RES_ERROR;
	}

	/* write data to flash */
	num_write = GET_NUM_BLOCK(CONFIG_FS_BLOCK_SIZE,
				  CONFIG_FS_FLASH_MAX_RW_SIZE);

	for (uint32_t i = 0; i < num_write; i++) {
		/* flash_write reenabled write-protection so disable it again */
		flash_write_protection_set(flash_dev, false);

		if (flash_write(flash_dev, fl_addr, src,
				CONFIG_FS_FLASH_MAX_RW_SIZE) != 0) {
			return RES_ERROR;
		}

		fl_addr += CONFIG_FS_FLASH_MAX_RW_SIZE;
		src += CONFIG_FS_FLASH_MAX_RW_SIZE;
	}

	return RES_OK;
}

DRESULT fat_disk_write(const void *buff, uint32_t start_sector,
		       uint32_t sector_count)
{
	off_t fl_addr;
	uint32_t remaining;
	uint32_t size;

	fl_addr = lba_to_address(start_sector);
	remaining = (sector_count * _MIN_SS);

	/* check if start address is erased-aligned address  */
	if (fl_addr & (CONFIG_FS_FLASH_ERASE_ALIGNMENT - 1)) {

		/* not aligned */
		/* check if the size goes over flash block boundary */
		if ((fl_addr + remaining) <
		    ((fl_addr + CONFIG_FS_BLOCK_SIZE) &
		     ~(CONFIG_FS_BLOCK_SIZE - 1))) {
			/* not over block boundary (a partial block also) */
			if (update_flash_block(fl_addr, remaining, buff) != 0) {
				return RES_ERROR;
			}
			return RES_OK;
		}

		/* write goes over block boundary */
		size = GET_SIZE_TO_BOUNDARY(fl_addr, CONFIG_FS_BLOCK_SIZE);

		/* write first partial block */
		if (update_flash_block(fl_addr, size, buff) != 0) {
			return RES_ERROR;
		}

		fl_addr += size;
		remaining -= size;
		buff += size;
	}

	/* start is an erase-aligned address */
	while (remaining) {
		if (remaining < CONFIG_FS_BLOCK_SIZE) {
			break;
		}

		if (update_flash_block(fl_addr, CONFIG_FS_BLOCK_SIZE,
					buff) != 0) {
			return RES_ERROR;
		}

		fl_addr += CONFIG_FS_BLOCK_SIZE;
		remaining -= CONFIG_FS_BLOCK_SIZE;
		buff += CONFIG_FS_BLOCK_SIZE;
	}

	/* remaining partial block */
	if (remaining) {
		if (update_flash_block(fl_addr, remaining, buff) != 0) {
			return RES_ERROR;
		}
	}

	return RES_OK;
}

DRESULT fat_disk_ioctl(uint8_t cmd, void *buff)
{
	switch (cmd) {
	case CTRL_SYNC:
		return RES_OK;
	case GET_SECTOR_COUNT:
		*(uint32_t *)buff = CONFIG_FS_VOLUME_SIZE / _MIN_SS;
		return RES_OK;
	case GET_BLOCK_SIZE: /* in sectors */
		*(uint32_t *)buff = CONFIG_FS_BLOCK_SIZE / _MIN_SS;
		return RES_OK;
	case CTRL_TRIM:
		break;
	}

	return RES_PARERR;
}
