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

#if defined(CONFIG_FILE_SYSTEM_MAX_FILE_NAME) && (CONFIG_FILE_SYSTEM_MAX_FILE_NAME - 0) > 0

/* No in-tree file system supports name longer than 255 characters */
#if (CONFIG_FILE_SYSTEM_LITTLEFS || CONFIG_FAT_FILESYSTEM_ELM ||	\
	CONFIG_FILE_SYSTEM_EXT2) && (CONFIG_FILE_SYSTEM_MAX_FILE_NAME > 255)
#error "Max allowed CONFIG_FILE_SYSTEM_MAX_FILE_NAME is 255 characters, when any in-tree FS enabled"
#endif

/* Enabled FAT driver, without LFN, restricts name length to 12 characters */
#if defined(CONFIG_FAT_FILESYSTEM_ELM) && !(CONFIG_FS_FATFS_LFN) && \
	(CONFIG_FILE_SYSTEM_MAX_FILE_NAME > 12)
#error "CONFIG_FILE_SYSTEM_MAX_FILE_NAME can not be > 12 if FAT is enabled without LFN"
#endif

#define MAX_FILE_NAME CONFIG_FILE_SYSTEM_MAX_FILE_NAME

#else
/* Select from enabled file systems */

#if defined(CONFIG_FAT_FILESYSTEM_ELM)

#if defined(CONFIG_FS_FATFS_LFN)
#define MAX_FILE_NAME CONFIG_FS_FATFS_MAX_LFN
#else /* CONFIG_FS_FATFS_LFN */
#define MAX_FILE_NAME 12 /* Uses 8.3 SFN */
#endif /* CONFIG_FS_FATFS_LFN */

#endif

#if !defined(MAX_FILE_NAME) && defined(CONFIG_FILE_SYSTEM_EXT2)
#define MAX_FILE_NAME 255
#endif

#if !defined(MAX_FILE_NAME) && defined(CONFIG_FILE_SYSTEM_LITTLEFS)
#define MAX_FILE_NAME 255
#endif

#if !defined(MAX_FILE_NAME) /* filesystem selection */
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
 * The object needs to be initialized with fs_file_t_init().
 */
struct fs_file_t {
	/** Pointer to file object structure */
	void *filep;
	/** Pointer to mount point structure */
	const struct fs_mount_t *mp;
	/** Open/create flags */
	fs_mode_t flags;
};

/**
 * @brief Directory object representing an open directory
 *
 * The object needs to be initialized with fs_dir_t_init().
 */
struct fs_dir_t {
	/** Pointer to directory object structure */
	void *dirp;
	/** Pointer to mount point structure */
	const struct fs_mount_t *mp;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_ */
