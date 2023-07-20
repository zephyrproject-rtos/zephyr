/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_FS_SYS_H_
#define ZEPHYR_INCLUDE_FS_FS_SYS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup file_system_api
 * @{
 */

/**
 * @brief File System interface structure
 */
struct fs_file_system_t {
	/**
	 * @name File operations
	 * @{
	 */
	/**
	 * Opens or creates a file, depending on flags given.
	 */
	int (*open)(struct fs_file_t *filp, const char *fs_path,
		    fs_mode_t flags);
	/**
	 * Reads nbytes number of bytes.
	 */
	ssize_t (*read)(struct fs_file_t *filp, void *dest, size_t nbytes);
	/**
	 * Writes nbytes number of bytes.
	 */
	ssize_t (*write)(struct fs_file_t *filp,
					const void *src, size_t nbytes);
	/**
	 * Moves the file position to a new location in the file.
	 */
	int (*lseek)(struct fs_file_t *filp, off_t off, int whence);
	/**
	 * Retrieves the current position in the file.
	 */
	off_t (*tell)(struct fs_file_t *filp);
	/**
	 * Truncates/expands the file to the new length.
	 */
	int (*truncate)(struct fs_file_t *filp, off_t length);
	/**
	 * Flushes the cache of an open file.
	 */
	int (*sync)(struct fs_file_t *filp);
	/**
	 * Flushes the associated stream and closes the file.
	 */
	int (*close)(struct fs_file_t *filp);
	/** @} */

	/**
	 * @name Directory operations
	 * @{
	 */
	/**
	 * Opens an existing directory specified by the path.
	 */
	int (*opendir)(struct fs_dir_t *dirp, const char *fs_path);
	/**
	 * Reads directory entries of an open directory.
	 */
	int (*readdir)(struct fs_dir_t *dirp, struct fs_dirent *entry);
	/**
	 * Closes an open directory.
	 */
	int (*closedir)(struct fs_dir_t *dirp);
	/** @} */

	/**
	 * @name File system level operations
	 * @{
	 */
	/**
	 * Mounts a file system.
	 */
	int (*mount)(struct fs_mount_t *mountp);
	/**
	 * Unmounts a file system.
	 */
	int (*unmount)(struct fs_mount_t *mountp);
	/**
	 * Deletes the specified file or directory.
	 */
	int (*unlink)(struct fs_mount_t *mountp, const char *name);
	/**
	 * Renames a file or directory.
	 */
	int (*rename)(struct fs_mount_t *mountp, const char *from,
					const char *to);
	/**
	 * Creates a new directory using specified path.
	 */
	int (*mkdir)(struct fs_mount_t *mountp, const char *name);
	/**
	 * Checks the status of a file or directory specified by the path.
	 */
	int (*stat)(struct fs_mount_t *mountp, const char *path,
					struct fs_dirent *entry);
	/**
	 * Returns the total and available space on the file system volume.
	 */
	int (*statvfs)(struct fs_mount_t *mountp, const char *path,
					struct fs_statvfs *stat);
#if defined(CONFIG_FILE_SYSTEM_MKFS) || defined(__DOXYGEN__)
	/**
	 * Formats a device to specified file system type.
	 * Available only if @kconfig{CONFIG_FILE_SYSTEM_MKFS} is enabled.
	 *
	 * @note This operation destroys existing data on the target device.
	 */
	int (*mkfs)(uintptr_t dev_id, void *cfg, int flags);
#endif
	/** @} */
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FS_SYS_H_ */
