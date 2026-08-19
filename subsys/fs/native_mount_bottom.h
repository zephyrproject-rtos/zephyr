/*
 * Copyright (c) 2025 Golioth, Inc.
 * Copyright (c) 2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_FS_NATIVE_MOUNT_BOTTOM_H
#define ZEPHYR_FS_NATIVE_MOUNT_BOTTOM_H

#include <stdint.h>

#define NATIVE_MOUNT_MAX_FILE_NAME_LEN 255

/** Mid-layer whence values for nsmb_lseek(), independent of host definitions */
#define NSMB_SEEK_MID_SET 0
#define NSMB_SEEK_MID_CUR 1
#define NSMB_SEEK_MID_END 2

struct fs_mid_dir;

enum fs_mid_dir_entry_type {
	/** Identifier for file entry */
	FS_MID_DIR_ENTRY_FILE = 0,
	/** Identifier for directory entry */
	FS_MID_DIR_ENTRY_DIR
};

struct fs_mid_dirent {
	/**
	 * File/directory type (FS_DIR_ENTRY_FILE or FS_DIR_ENTRY_DIR)
	 */
	enum fs_mid_dir_entry_type type;
	/** Name of file or directory */
	char name[NATIVE_MOUNT_MAX_FILE_NAME_LEN + 1];
	/** Size of file (0 if directory). */
	uint64_t size;
};

struct fs_mid_statvfs {
	/** Optimal transfer block size */
	unsigned long f_bsize;
	/** Allocation unit size */
	unsigned long f_frsize;
	/** Size of FS in f_frsize units */
	unsigned long f_blocks;
	/** Number of free blocks */
	unsigned long f_bfree;
};

int nsmb_open(const char *prefix, const char *path, int flags_mid);
int nsmb_stat(const char *prefix, const char *path, struct fs_mid_dirent *dirent_mid);
int nsmb_statvfs(const char *prefix, const char *path, struct fs_mid_statvfs *stat_mid);
long long nsmb_lseek(int fd, long long offset, int whence_mid);

int nsmb_mkdir(const char *prefix, const char *path);
struct fs_mid_dir *nsmb_opendir(const char *prefix, const char *name);
int nsmb_readdir(struct fs_mid_dir *dir, struct fs_mid_dirent *entry_mid);
int nsmb_closedir(struct fs_mid_dir *dir);

int nsmb_unlink(const char *prefix, const char *path);
int nsmb_rename(const char *prefix, const char *from, const char *to);

#endif /* ZEPHYR_FS_NATIVE_MOUNT_BOTTOM_H */
