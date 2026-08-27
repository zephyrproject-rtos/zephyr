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
 * @p All parameters (ctx, file_path and disk_access_name) must point to data that will remain valid
 * until the disk access is unregistered. This is trivially true for file_path and disk_access_name
 * if they are string literals, but care must be taken for ctx, as well as for file_path and
 * disk_access_name if they are constructed dynamically.
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

/**
 * @brief Unregister a previously registered loopback disk device
 *
 * Cleans up resources used by the disk access.
 *
 * @param ctx Context structure previously passed to a successful invocation of
 * loopback_disk_access_register()
 *
 * @retval 0 on success;
 * @retval <0 negative errno code, depending on file system of the backing file.
 */
int loopback_disk_access_unregister(struct loopback_disk_access *ctx);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LOOPBACK_DISK_ACCESS_H_ */
