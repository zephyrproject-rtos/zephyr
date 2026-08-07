/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ZVFS_FS_H_
#define ZEPHYR_INCLUDE_SYS_ZVFS_FS_H_

/**
 * @file
 * @brief ZVFS path open via the Zephyr file system.
 *
 * Provides zvfs_open(), which wraps Zephyr fs_open() and registers the result in
 * the fd table. ISO C fopen() and POSIX open() both use this entry point; it is
 * not part of the POSIX_FILE_SYSTEM option group.
 */

#include <stdbool.h>

#include <zephyr/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Backing object for a ZVFS file or directory stream on a mounted FS.
 *
 * Used by zvfs_open() for files (registered in the fd table) and by POSIX
 * opendir() for directory streams (not fd-backed).
 */
struct zvfs_fs_desc {
	union {
		struct fs_file_t file;
		struct fs_dir_t dir;
	};
	/** Open flags (ZVFS_O_*) reported by F_GETFL */
	int flags;
	bool is_dir: 1;
	bool used: 1;
};

/**
 * @brief Open a path on a mounted Zephyr file system and return an fd.
 *
 * @param name Absolute path (as required by fs_open()).
 * @param flags POSIX O_* open flags.
 * @param mode File mode when O_CREAT is set.
 *
 * @return File descriptor on success, or -1 on error (errno is set).
 */
int zvfs_open(const char *name, int flags, int mode);

/**
 * @brief Allocate a ZVFS FS descriptor slot.
 *
 * @param is_dir True for a directory stream, false for a file object.
 *
 * @return Pointer to a descriptor, or NULL if the pool is exhausted.
 */
struct zvfs_fs_desc *zvfs_fs_desc_alloc(bool is_dir);

/**
 * @brief Return a ZVFS FS descriptor slot to the pool.
 *
 * @param ptr Descriptor previously returned by zvfs_fs_desc_alloc().
 */
void zvfs_fs_desc_free(struct zvfs_fs_desc *ptr);

/**
 * @brief Drop POSIX record locks associated with a ZVFS FS object.
 *
 * @param obj Open file description object (zvfs_fs_desc pointer).
 */
void zvfs_record_lock_drop_obj(void *obj);

/**
 * @brief Delete file or directory.
 *
 * @param path Path or name of file to be removed.
 *
 * @return 0 on success, -1 with errno set on error.
 */
int zvfs_unlink(const char *path);

/**
 * @brief Rename file or directory, moving it if necessary.
 *
 * @param old Current name
 * @param newp Target name
 *
 * @return 0 on success, -1 with errno set on error.
 */
int zvfs_rename(const char *old, const char *newp);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ZVFS_FS_H_ */
