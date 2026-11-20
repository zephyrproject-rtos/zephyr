/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Disk Access API for virtual FAT disks.
 * @ingroup virtual_fat_disk_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISK_VIRTUAL_FAT_DISK_H
#define ZEPHYR_INCLUDE_DRIVERS_DISK_VIRTUAL_FAT_DISK_H

#include <zephyr/fs/virtual_fat.h>

#include <zephyr/device.h>

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup virtual_fat_disk_interface Virtual FAT Disk Access
 * @ingroup disk_driver_interface_ext
 * @since 4.5
 * @version 0.1.0
 * @{
 */

/**
 * @brief register files that are shown in the mass storage device
 *
 * This function accepts an array with virtual files that should be displayed
 * then the mass storage device is mounted by the host.
 * When the device is mounted, this array must not change!
 *
 * @param dev        disk device instance
 * @param files      array of virtual files
 * @param files_len  length of the array
 * @retval 0 on success, negative errno code otherwise
 */
int disk_register_virtual_files(const struct device *dev, const struct virtual_file *files,
				size_t files_len);

/**
 * @brief register a callback that is called when data for a new file is written
 *
 * @param dev       disk device instance
 * @param callback   function that is called when data is written to a new file
 * @param user_data this data is passed to the callback when it is called
 */
void disk_register_new_file_write_cb(const struct device *dev, put_file_content_chunk_t callback,
				     void *user_data);

/**
 * @brief set the volume label
 *
 * This function sets the volume label (first file in the root directory) to
 * a given string. The maximum length is 11. The string is capped if it is too long
 *
 * @param dev        disk device instance
 * @param label      new label
 */
void disk_set_volume_label(const struct device *dev, const char *label);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISK_VIRTUAL_FAT_DISK_H */
