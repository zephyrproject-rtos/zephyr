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
struct zvfs_dirstream;

struct zvfs_fs_desc {
	union {
		struct fs_file_t file;
		struct fs_dir_t dir;
	};
	/** Stream object returned by zvfs_opendir(), NULL for plain fds. */
	struct zvfs_dirstream *stream;
#if defined(CONFIG_ZVFS_FS) || defined(__DOXYGEN__)
	/** Resolved absolute path of directory descriptors, for zvfs_fchdir(). */
	char path[CONFIG_ZVFS_PATH_MAX];
#endif
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
 * Longest directory entry name zvfs_readdir() reports. A fixed constant, not
 * MAX_FILE_NAME, so struct zvfs_dirent has a configuration-independent layout
 * that POSIX struct dirent can mirror; longer backend names are truncated.
 */
#define ZVFS_NAME_MAX 255

/**
 * @brief Directory entry as reported by zvfs_readdir().
 */
struct zvfs_dirent {
	uint32_t d_type;                 /**< ZVFS_MODE_IFDIR or ZVFS_MODE_IFREG */
	unsigned long d_ino;             /**< Serial number, 0 (untracked) */
	size_t d_size;                   /**< Entry size in bytes, 0 for directories */
	char d_name[ZVFS_NAME_MAX + 1];  /**< NUL-terminated entry name */
};

/**
 * @brief Directory stream state backing a POSIX DIR.
 *
 * Allocated by zvfs_opendir() from storage that is user-accessible under
 * CONFIG_USERSPACE (zvfs_dir_partition) and released when the stream's file
 * descriptor is closed.
 */
struct zvfs_dirstream {
	int fd;                  /**< Directory file descriptor (a K_OBJ_FILE) */
	struct zvfs_dirent ent;  /**< Storage for the current entry */
};

/**
 * @brief Open a directory stream.
 *
 * The stream's descriptor closes through zvfs_close(), which also releases
 * the stream object.
 *
 * @param path Absolute path of the directory.
 *
 * @return Stream pointer on success, or NULL on error (errno is set).
 */
__syscall struct zvfs_dirstream *zvfs_opendir(const char *path);

/**
 * @brief Read the next directory entry.
 *
 * @param fd Directory file descriptor.
 * @param ent Destination for the entry.
 *
 * @retval 0 when an entry was read;
 * @retval 1 at end of directory (@p ent is not modified, errno is unchanged);
 * @retval -1 on error (errno is set).
 */
__syscall int zvfs_readdir(int fd, struct zvfs_dirent *ent);

/**
 * @brief Reset a directory stream to its first entry.
 *
 * @param fd Directory file descriptor.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_rewinddir(int fd);

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
 * @brief Find an executable along a directory list
 *
 * Search the colon-separated directory list @p paths for a regular file named
 * @p exe (which must not contain a path separator) with an execute permission
 * bit. An empty entry in the list is the working directory, and a match there
 * is reported by its bare name. Each candidate's path is written to @p buf and
 * opened by it; the first match is left open and its descriptor returned. The
 * list is read, never written: a caller passing an environment variable
 * synchronizes its own modifications of the environment.
 *
 * @param paths    Colon-separated directory list.
 * @param exe      Name to find, without a path separator.
 * @param buf      Where to write the resolved path.
 * @param buf_size Size of @p buf.
 *
 * @retval fd       The open descriptor of the executable (non-negative).
 * @retval -ENOENT  No entry of that name along the list.
 * @retval -EPERM   Only entries that are not executable regular files were found.
 * @retval -ERANGE  A candidate path does not fit in @p buf.
 * @retval -EINVAL  A NULL list, name or buffer, an empty name or buffer, or a
 *                  separator in the name.
 * @retval -errno   Another failure opening or inspecting a candidate.
 */
int zvfs_exe_resolve(const char *paths, const char *exe, char *buf, size_t buf_size);

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
 * @brief Set the timestamps of an open file or directory.
 *
 * @param fd Descriptor of the file or directory.
 * @param times Pointer to two timespecs, access time first and modification
 *              time second, or NULL to set both to the current time.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_futime(int fd, const struct timespec *times);

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

/**
 * @brief Change the ZVFS current working directory.
 *
 * The working directory is system-wide, matching POSIX per-process semantics
 * on a single-process system, and seeds relative path resolution for every
 * ZVFS path operation.
 *
 * @param path Absolute or relative path of an existing directory.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_chdir(const char *path);

/**
 * @brief Change the ZVFS current working directory to an open directory.
 *
 * @param fd Directory file descriptor from zvfs_opendir() or
 *           zvfs_open() with ZVFS_O_DIRECTORY.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_fchdir(int fd);

/**
 * @brief Copy the ZVFS current working directory into @p buf.
 *
 * @param buf Destination buffer.
 * @param size Size of @p buf; ERANGE when the path does not fit.
 *
 * @return 0 on success, -1 with errno set on error.
 */
__syscall int zvfs_getcwd(char *buf, size_t size);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/zvfs_fs.h>

#endif /* ZEPHYR_INCLUDE_SYS_ZVFS_FS_H_ */
