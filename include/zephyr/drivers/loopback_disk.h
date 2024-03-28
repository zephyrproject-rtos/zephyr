/*
 * Copyright (c) 2023 Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LOOPBACK_DISK_ACCESS_H_
#define ZEPHYR_INCLUDE_DRIVERS_LOOPBACK_DISK_ACCESS_H_

#include <zephyr/drivers/disk.h>
#include <zephyr/fs/fs_interface.h>

/**
 * @brief Context object for an active loopback disk device
 */
struct loopback_disk_access {
	struct disk_info info;
	const char *file_path;
	struct fs_file_t file;
	size_t num_sectors;
};

/**
 * @brief Register a loopback disk device
 *
 * Registers a new loopback disk deviced backed by a file at the specified path.
 *
 * @details
 * @p ctx must be a pointer to a "struct loopback_disk_access" structure that will remain valid for
 * as long as the disk access is in use.
 *
 * @param ctx Preallocated context structure
 * @param file_path Path to backing file
 * @param disk_access_name Name of the created disk access (for disk_access_*() functions)
 *
 * @retval 0 on success;
 * @retval <0 negative errno code, depending on file system of the backing file.
 */
int loopback_disk_access_register(struct loopback_disk_access *ctx, const char *file_path,
				  const char *disk_access_name);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LOOPBACK_DISK_ACCESS_H_ */
