/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal helpers shared by the ZVFS file system translation units.
 */

#ifndef ZEPHYR_LIB_OS_ZVFS_ZVFS_FS_PRIV_H_
#define ZEPHYR_LIB_OS_ZVFS_ZVFS_FS_PRIV_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fs_statvfs;
struct zvfs_statvfs;

/**
 * @brief Map Zephyr file system statistics into ZVFS statistics.
 *
 * @param buf Destination.
 * @param vfs Source, from fs_statvfs().
 * @param rdonly True when the containing mount is read-only.
 */
void zvfs_statvfs_from_fs(struct zvfs_statvfs *buf, const struct fs_statvfs *vfs, bool rdonly);
void zvfs_statvfs_root(struct zvfs_statvfs *buf);

/* the mode every stat of a file system entry reports: type plus synthesized permissions */
uint32_t zvfs_synth_mode(bool is_dir, bool rdonly);

#ifdef CONFIG_USERSPACE
/**
 * @brief Copy a user-supplied path into a kernel buffer of CONFIG_ZVFS_PATH_MAX bytes.
 *
 * An over-long path is an error to report (-1 with errno ENAMETOOLONG); an
 * invalid pointer is a fault to oops on, per the syscall conventions.
 *
 * @param dst Kernel destination of at least CONFIG_ZVFS_PATH_MAX bytes.
 * @param src User-supplied path.
 *
 * @return 0 on success, -1 with errno set on error.
 */
int zvfs_copy_user_path(char *dst, const char *src);
#endif /* CONFIG_USERSPACE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_OS_ZVFS_ZVFS_FS_PRIV_H_ */
