/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>

#include "ext2.h"
#include "ext2_mem.h"
#include "ext2_flash.h"
#include "ext2_struct.h"

LOG_MODULE_DECLARE(ext2);

static void flash_close_backend(struct ext2_data *fs);
static int64_t flash_device_size(struct ext2_data *fs);
static int64_t flash_write_size(struct ext2_data *fs);
static int flash_read_page(struct ext2_data *fs, void *buf, uint32_t page);
static int flash_write_page(struct ext2_data *fs, const void *buf, uint32_t page);
static int flash_read_superblock(struct ext2_data *fs, struct disk_superblock *sb);

int flash_init_backend(struct ext2_data *fs, const void *storage_dev, int flags)
{
	int rc;
	const struct flash_area *fap;
	unsigned int id = (uintptr_t)storage_dev;

	rc = flash_area_open(id, &fap);
	if (rc < 0) {
		LOG_ERR("FAIL: unable to find flash area %u: %d\n", id, rc);
		return rc;
	}

	fs->backend = (void *)fap;

	fs->close_backend = flash_close_backend;
	fs->get_device_size = flash_device_size;
	fs->get_write_size = flash_write_size;
	fs->read_page = flash_read_page;
	fs->write_page = flash_write_page;
	fs->read_superblock = flash_read_superblock;

	return 0;
}

void flash_close_backend(struct ext2_data *fs)
{
	const struct flash_area *fap = fs->backend;

	flash_area_close(fap);
}

static int64_t flash_device_size(struct ext2_data *fs)
{
	const struct flash_area *fap = fs->backend;

	return fap->fa_size;
}

static int64_t flash_write_size(struct ext2_data *fs)
{
	const struct flash_area *fap = fs->backend;
	const struct device *dev = flash_area_get_device(fap);
	const struct flash_driver_api *fapi = dev->api;
	const struct flash_pages_layout *pl;
	size_t lsize;

	fapi->page_layout(dev, &pl, &lsize);

	return pl->pages_size;
}

#define FLASH_PAGE_SIZE 4096

static int flash_read_page(struct ext2_data *fs, void *buf, uint32_t page)
{
	int rc;
	const struct flash_area *fap = fs->backend;
	uint32_t offset = page * FLASH_PAGE_SIZE;

	rc = flash_area_read(fap, offset, buf, FLASH_PAGE_SIZE);
	if (rc < 0) {
		LOG_ERR("FAIL: read from flash (%d) at %d", rc, offset);
		return rc;
	}

	return 0;
}

static int flash_write_page(struct ext2_data *fs, const void *buf, uint32_t page)
{
	int rc;
	const struct flash_area *fap = fs->backend;
	uint32_t offset = page * FLASH_PAGE_SIZE;

	rc = flash_area_erase(fap, offset, FLASH_PAGE_SIZE);
	if (rc < 0) {
		LOG_ERR("FAIL: flash erase addr 0x%x", page * FLASH_PAGE_SIZE);
		return rc;
	}

	rc = flash_area_write(fap, offset, buf, FLASH_PAGE_SIZE);
	if (rc < 0) {
		LOG_ERR("FAIL: write to flash addr %x", page * FLASH_PAGE_SIZE);
		return rc;
	}
	return 0;
}

static int flash_read_superblock(struct ext2_data *fs, struct disk_superblock *sb)
{
	int rc;
	const struct flash_area *fap = fs->backend;

	rc = flash_area_read(fap, EXT2_SUPERBLOCK_OFFSET, sb, sizeof(struct disk_superblock));
	if (rc < 0) {
		LOG_ERR("FAIL: read superblock from flash (%d)", rc);
		return rc;
	}

	return 0;
}
