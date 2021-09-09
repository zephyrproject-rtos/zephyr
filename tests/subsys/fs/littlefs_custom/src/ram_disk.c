/*
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "ram_disk.h"

#define RAM_DISK_BLOCK_SIZE 4096

static uint8_t ram_disk_storage[16*RAM_DISK_BLOCK_SIZE];
static uintptr_t ram_disk_context;

int ram_disk_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	void *buffer, lfs_size_t size)
{
	size_t complete_offset = block * c->block_size + off;

	if (complete_offset + size > sizeof(ram_disk_storage)) {
		return LFS_ERR_NOSPC;
	}

	memcpy(buffer, ram_disk_storage + complete_offset, size);

	return LFS_ERR_OK;
}

int ram_disk_program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	const void *buffer, lfs_size_t size)
{
	size_t complete_offset = block * c->block_size + off;

	if (complete_offset + size > sizeof(ram_disk_storage)) {
		return LFS_ERR_NOSPC;
	}

	memcpy(ram_disk_storage + complete_offset, buffer, size);

	return LFS_ERR_OK;
}

int ram_disk_erase(const struct lfs_config *c, lfs_block_t block)
{
	size_t complete_offset = block * c->block_size;

	if (complete_offset + c->block_size > sizeof(ram_disk_storage)) {
		return LFS_ERR_NOSPC;
	}

	memset(ram_disk_storage + complete_offset, 0, c->block_size);

	return LFS_ERR_OK;
}

int ram_disk_sync(const struct lfs_config *c)
{
	return LFS_ERR_OK;
}

int ram_disk_open(void **context, uintptr_t area_id)
{
	ram_disk_context = area_id;
	*context = &ram_disk_context;
	return 0;
}

void ram_disk_close(void *context)
{
}

size_t ram_disk_get_block_size(void *context)
{
	return RAM_DISK_BLOCK_SIZE;
}

size_t ram_disk_get_size(void *context)
{
	return sizeof(ram_disk_storage);
}
