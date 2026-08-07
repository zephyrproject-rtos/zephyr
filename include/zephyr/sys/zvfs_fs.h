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
#include <time.h>

#include <zephyr/fs/fs.h>
#include <zephyr/sys/zvfs.h>
#include <zephyr/toolchain.h>

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

/** @name zvfs_access() amode bits */
/**@{*/
#define ZVFS_F_OK 0 /**< Existence only */
#define ZVFS_X_OK 1 /**< Execute or search permission */
#define ZVFS_W_OK 2 /**< Write permission */
#define ZVFS_R_OK 4 /**< Read permission */
/**@}*/

/** @name zvfs_statvfs f_flag bits */
/**@{*/
#define ZVFS_ST_RDONLY 0x1 /**< Read-only file system */
#define ZVFS_ST_NOSUID 0x2 /**< Setuid/setgid bits are ignored */
/**@}*/

/**
 * @brief File system statistics for zvfs_statvfs() and zvfs_fstatvfs().
 */
struct zvfs_statvfs {
	unsigned long f_bsize;   /**< File system block size */
	unsigned long f_frsize;  /**< Fundamental file system block size */
	unsigned long f_blocks;  /**< Total number of blocks (in f_frsize units) */
	unsigned long f_bfree;   /**< Number of free blocks */
	unsigned long f_bavail;  /**< Number of free blocks available to all callers */
	unsigned long f_files;   /**< Total number of file serial numbers, 0 when untracked */
	unsigned long f_ffree;   /**< Number of free file serial numbers, 0 when untracked */
	unsigned long f_favail;  /**< Free file serial numbers available, 0 when untracked */
	unsigned long f_fsid;    /**< File system ID, 0 when untracked */
	unsigned long f_flag;    /**< ZVFS_ST_* flag bits */
	unsigned long f_namemax; /**< Maximum filename length */
	/** Timestamp resolution in nanoseconds, 0 when unknown. */
	unsigned long f_timestamp_resolution;
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
__syscall int zvfs_open(const char *name, int flags, int mode);

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
__syscall int zvfs_unlink(const char *path);

/**
 * @brief Rename file or directory, moving it if necessary.
 *
 * @param old Current name
 * @param newp Target name
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_rename(const char *old, const char *newp);

/**
 * @brief Delete a file, or an empty directory, as ISO C remove() requires.
 *
 * @param path Path to remove.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_remove(const char *path);

/**
 * @brief Get status of a file or directory by path.
 *
 * @param path Path to query.
 * @param buf Destination for the status.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_stat(const char *path, struct zvfs_stat *buf);

/**
 * @brief Get statistics of the file system containing @p path.
 *
 * @param path Any path on the mounted file system.
 * @param buf Destination for the statistics.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_statvfs(const char *path, struct zvfs_statvfs *buf);

/**
 * @brief Get statistics of the file system containing the open file @p fd.
 *
 * @param fd File descriptor of an open file.
 * @param buf Destination for the statistics.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_fstatvfs(int fd, struct zvfs_statvfs *buf);

/**
 * @brief Create a directory.
 *
 * @param path Path of the directory to create.
 * @param mode Permission bits, accepted and ignored (Zephyr file systems
 *             store no permissions).
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_mkdir(const char *path, uint32_t mode);

/**
 * @brief Remove a directory.
 *
 * @param path Path of the directory to remove.
 *
 * @return 0 on success, -1 with errno set on error (ENOTDIR when @p path
 *         names a regular file).
 */
__syscall int zvfs_rmdir(const char *path);

/**
 * @brief Truncate a file to a given length by path.
 *
 * @param path Path of the file.
 * @param length New length in bytes.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_truncate(const char *path, off_t length);

/**
 * @brief Check accessibility of a file or directory.
 *
 * @param path Path to check.
 * @param amode ZVFS_F_OK, or any bitwise combination of ZVFS_R_OK,
 *              ZVFS_W_OK, and ZVFS_X_OK.
 *
 * @return 0 when access would be granted, -1 with errno set otherwise.
 */
__syscall int zvfs_access(const char *path, int amode);

/**
 * @brief Set file or directory timestamps.
 *
 * @param path Path of the file or directory.
 * @param times Pointer to two timespecs, access time first and modification
 *              time second, or NULL to set both to the current time.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_utime(const char *path, const struct timespec *times);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/zvfs_fs.h>

#endif /* ZEPHYR_INCLUDE_SYS_ZVFS_FS_H_ */
