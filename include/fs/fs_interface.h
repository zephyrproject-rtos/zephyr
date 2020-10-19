/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_
#define ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (CONFIG_FILE_SYSTEM_MAX_FILE_NAME - 0) > 0
#define MAX_FILE_NAME CONFIG_FILE_SYSTEM_MAX_FILE_NAME
#else /* CONFIG_FILE_SYSTEM_MAX_FILE_NAME */
/* Select from enabled file systems */
#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)
#define MAX_FILE_NAME 256
#elif defined(CONFIG_FAT_FILESYSTEM_ELM)
#if defined(CONFIG_FS_FATFS_LFN)
#define MAX_FILE_NAME CONFIG_FS_FATFS_MAX_LFN
#else /* CONFIG_FS_FATFS_LFN */
#define MAX_FILE_NAME 12 /* Uses 8.3 SFN */
#endif /* CONFIG_FS_FATFS_LFN */
#else /* filesystem selection */
/* Use standard 8.3 when no filesystem is explicitly selected */
#define MAX_FILE_NAME 12
#endif /* filesystem selection */
#endif /* CONFIG_FILE_SYSTEM_MAX_FILE_NAME */


/* Type for fs_open flags */
typedef uint8_t fs_mode_t;

struct fs_mount_t;

/**
 * @addtogroup file_system_api
 * @{
 */

/**
 * @brief File object representing an open file
 *
 * @param Pointer to FATFS file object structure
 * @param mp Pointer to mount point structure
 */
struct fs_file_t {
	void *filep;
	const struct fs_mount_t *mp;
	fs_mode_t flags;
};

/**
 * @brief Directory object representing an open directory
 *
 * @param dirp Pointer to directory object structure
 * @param mp Pointer to mount point structure
 */
struct fs_dir_t {
	void *dirp;
	const struct fs_mount_t *mp;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_ */
