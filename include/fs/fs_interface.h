/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FS_INTERFACE_H_
#define _FS_INTERFACE_H_

#ifdef CONFIG_FAT_FILESYSTEM_ELM
#include <ff.h>
#endif

#ifdef CONFIG_FILE_SYSTEM_NFFS
#include <nffs/nffs.h>
#endif

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
 * @param fatfs_fp FATFS file object structure
 * @param nffs_fp NFFS file object structure
 * @param mp Pointer to mount point structure
 */
struct fs_file_t {
	union {
#ifdef CONFIG_FAT_FILESYSTEM_ELM
		FIL fatfs_fp;
#endif
#ifdef CONFIG_FILE_SYSTEM_NFFS
		struct nffs_file *nffs_fp;
#endif
	};
	const struct fs_mount_t *mp;
};

/**
 * @brief Directory object representing an open directory
 *
 * @param fatfs_dp FATFS directory object structure
 * @param nffs_dp NFFS directory object structure
 * @param mp Pointer to mount point structure
 */
struct fs_dir_t {
	union {
#ifdef CONFIG_FAT_FILESYSTEM_ELM
		DIR fatfs_dp;
#endif
#ifdef CONFIG_FILE_SYSTEM_NFFS
		struct nffs_dir *nffs_dp;
#endif
	};
	const struct fs_mount_t *mp;
};

#ifdef __cplusplus
}
#endif

#endif /* _FS_INTERFACE_H_ */
