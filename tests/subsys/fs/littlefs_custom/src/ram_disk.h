/*
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_CUSTOM_RAM_DISK_H_
#define  _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_CUSTOM_RAM_DISK_H_

#include <stddef.h>
#include <fs/littlefs.h>

int ram_disk_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	void *buffer, lfs_size_t size);
int ram_disk_program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	const void *buffer, lfs_size_t size);
int ram_disk_erase(const struct lfs_config *c, lfs_block_t block);
int ram_disk_sync(const struct lfs_config *c);
int ram_disk_open(void **context, uintptr_t area_id);
void ram_disk_close(void *context);
size_t ram_disk_get_block_size(void *context);
size_t ram_disk_get_size(void *context);

#endif /* _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_CUSTOM_RAM_DISK_H_ */
