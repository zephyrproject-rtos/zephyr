/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TESTS_FS_EXT2_UTILS_H__
#define __TESTS_FS_EXT2_UTILS_H__

extern struct fs_mount_t testfs_mnt;

int wipe_partition(uintptr_t id);
size_t get_partition_size(uintptr_t id);

#endif /* __TESTS_FS_EXT2_UTILS_H__ */
