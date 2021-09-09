/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_LFS_H_
#define _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_LFS_H_

#include <fs/fs.h>
#include "testfs_util.h"

#define TESTFS_MNT_POINT_SMALL "/sml"
#define TESTFS_MNT_POINT_MEDIUM "/med"
#define TESTFS_MNT_POINT_LARGE "/lrg"
#define TESTFS_MNT_POINT_RAM_DISK "/ram"

extern struct fs_mount_t testfs_small_mnt;
extern struct fs_mount_t testfs_medium_mnt;
extern struct fs_mount_t testfs_large_mnt;
extern struct fs_mount_t testfs_ram_disk_mnt;
extern struct fs_mount_t testfs_ram_disk_default_mnt;

#define MEDIUM_IO_SIZE 64
#define MEDIUM_CACHE_SIZE 256
#define MEDIUM_LOOKAHEAD_SIZE 64

#define LARGE_IO_SIZE 256
#define LARGE_CACHE_SIZE 1024
#define LARGE_LOOKAHEAD_SIZE 128

/** Wipe all data from the flash partition associated with the given
 * mount point.
 *
 * This will cause it to be reformatted the next time it is mounted.
 *
 * @param mp pointer to the mount point data.  The flash partition ID
 * must be encoded in the fs_mount_t::storage_dev pointer.
 *
 * @return a standard test result code.
 */
int testfs_lfs_wipe_flash_partition(const struct fs_mount_t *mp);

/** Wipe all data from the ram disk device.
 *
 * This will cause it to be reformatted the next time it is mounted.
 *
 * @return a standard test result code.
 */
int testfs_lfs_wipe_ram_disk(void);

#endif /* _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_LFS_H_ */
