/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_
#define ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_FILE_SYSTEM_NFFS
#define MAX_FILE_NAME 256
#else /* FAT_FS */
#define MAX_FILE_NAME 12 /* Uses 8.3 SFN */
#endif

struct fs_mount_t;

/**
 * @brief File object representing an open file
 *
 * @param Pointer to FATFS file object structure
 * @param mp Pointer to mount point structure
 */
struct fs_file_t {
	void *filep;
	const struct fs_mount_t *mp;
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_ */
