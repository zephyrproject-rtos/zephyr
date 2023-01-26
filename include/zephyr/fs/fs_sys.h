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
 *
 * @param open Opens or creates a file, depending on flags given
 * @param read Reads nbytes number of bytes
 * @param write Writes nbytes number of bytes
 * @param lseek Moves the file position to a new location in the file
 * @param tell Retrieves the current position in the file
 * @param truncate Truncates/expands the file to the new length
 * @param sync Flushes the cache of an open file
 * @param close Flushes the associated stream and closes the file
 * @param opendir Opens an existing directory specified by the path
 * @param readdir Reads directory entries of an open directory
 * @param closedir Closes an open directory
 * @param mount Mounts a file system
 * @param unmount Unmounts a file system
 * @param unlink Deletes the specified file or directory
 * @param rename Renames a file or directory
 * @param mkdir Creates a new directory using specified path
 * @param stat Checks the status of a file or directory specified by the path
 * @param statvfs Returns the total and available space on the file system
 *        volume
 * @param mkfs Formats a device to specified file system type. Note that this
 *        operation destroys existing data on a target device.
 */
struct fs_file_system_t {
	/* File operations */
	int (*open)(struct fs_file_t *filp, const char *fs_path,
		    fs_mode_t flags);
	ssize_t (*read)(struct fs_file_t *filp, void *dest, size_t nbytes);
	ssize_t (*write)(struct fs_file_t *filp,
					const void *src, size_t nbytes);
	int (*lseek)(struct fs_file_t *filp, off_t off, int whence);
	off_t (*tell)(struct fs_file_t *filp);
	int (*truncate)(struct fs_file_t *filp, off_t length);
	int (*sync)(struct fs_file_t *filp);
	int (*close)(struct fs_file_t *filp);
	/* Directory operations */
	int (*opendir)(struct fs_dir_t *dirp, const char *fs_path);
	int (*readdir)(struct fs_dir_t *dirp, struct fs_dirent *entry);
	int (*closedir)(struct fs_dir_t *dirp);
	/* File system level operations */
	int (*mount)(struct fs_mount_t *mountp);
	int (*unmount)(struct fs_mount_t *mountp);
	int (*unlink)(struct fs_mount_t *mountp, const char *name);
	int (*rename)(struct fs_mount_t *mountp, const char *from,
					const char *to);
	int (*mkdir)(struct fs_mount_t *mountp, const char *name);
	int (*stat)(struct fs_mount_t *mountp, const char *path,
					struct fs_dirent *entry);
	int (*statvfs)(struct fs_mount_t *mountp, const char *path,
					struct fs_statvfs *stat);
#if defined(CONFIG_FILE_SYSTEM_MKFS)
	int (*mkfs)(uintptr_t dev_id, void *cfg, int flags);
#endif
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FS_SYS_H_ */
