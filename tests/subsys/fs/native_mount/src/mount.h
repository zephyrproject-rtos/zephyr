/*
 * Copyright (c) 2025 Golioth, Inc.
 * Copyright (c) 2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FS_MOUNT_H__
#define __FS_MOUNT_H__

#include <zephyr/fs/fs.h>

extern struct fs_mount_t test_mp;

int fs_rm_all(const char *path);

#endif /* __FS_MOUNT_H__ */
