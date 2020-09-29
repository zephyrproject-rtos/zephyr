/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <zephyr/types.h>
#include <errno.h>
#include <init.h>
#include <fs/fs.h>
#include <sys/__assert.h>
#include "test_fs.h"

#define BUF_LEN 128
static char buffer[BUF_LEN];
static char *read_pos = buffer;
static char *cur = buffer;
static int file_length;
static struct fs_mount_t *mp[FS_TYPE_EXTERNAL_BASE];
static bool nospace;

static
int temp_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t flags)
{
	if (zfp == NULL || file_name == NULL) {
		return -EINVAL;
	}

	if (zfp->filep) {
		if (strcmp(zfp->filep, file_name) == 0) {
			/* file has been opened */
			return -EEXIST;
		}
	}

	if (!(flags & FS_O_MASK)) {
		return -EINVAL;
	}

	zfp->filep = (char *)file_name;
	return 0;
}

static int temp_close(struct fs_file_t *zfp)
{
	if (zfp == NULL) {
		return -EINVAL;
	}

	if (zfp->filep) {
		zfp->filep = NULL;
	} else {
		return -EIO;
	}

	return 0;
}

static int temp_unlink(struct fs_mount_t *mountp, const char *path)
{
	if (mountp == NULL || path == NULL) {
		return -EINVAL;
	}

	if (strcmp(mountp->mnt_point, path) == 0) {
		return -EPERM;
	}
	return 0;
}

static int temp_rename(struct fs_mount_t *mountp, const char *from,
			const char *to)
{
	if (mountp == NULL || from == NULL || to == NULL) {
		return -EINVAL;
	}

	if (strcmp(to, TEST_FILE_EX) == 0) {
		return -EINVAL;
	}
	return 0;
}

static ssize_t temp_read(struct fs_file_t *zfp, void *ptr, size_t size)
{
	unsigned int br;

	if (zfp == NULL || ptr == NULL) {
		return -EINVAL;
	}

	br = size;
	if (read_pos - buffer + br > file_length) {
		br = file_length - (read_pos - buffer);
	}
	memcpy(ptr, read_pos, br);
	read_pos += br;
	cur = read_pos;

	return br;
}

static ssize_t temp_write(struct fs_file_t *zfp, const void *ptr, size_t size)
{
	unsigned int bw;

	if (zfp == NULL || ptr == NULL) {
		return -EINVAL;
	}

	if (nospace) {
		return -ENOSPC;
	}

	bw = size;
	if (file_length + bw >  BUF_LEN) {
		bw = BUF_LEN - file_length;
		nospace = true;
	}

	memcpy(buffer + file_length, ptr, bw);
	file_length += bw;
	cur = buffer + file_length;

	return bw;
}

static int temp_seek(struct fs_file_t *zfp, off_t offset, int whence)
{

	if (!zfp) {
		return -EINVAL;
	}

	switch (whence) {
	case FS_SEEK_SET:
		cur = buffer + offset;
		break;
	case FS_SEEK_CUR:
		cur += offset;
		break;
	case FS_SEEK_END:
		cur = buffer + file_length + offset;
		break;
	default:
		return -EINVAL;
	}

	if ((cur < buffer) || (cur > buffer + file_length)) {
		return -EINVAL;
	}

	return 0;
}

static off_t temp_tell(struct fs_file_t *zfp)
{
	if (!zfp) {
		return -EINVAL;
	}

	if (nospace) {
		return -ENOSPC;
	}

	return cur - buffer;
}

static int temp_truncate(struct fs_file_t *zfp, off_t length)
{
	if (!zfp) {
		return -EINVAL;
	}

	if (length > BUF_LEN) {
		return -EINVAL;
	}
	file_length = length;
	return 0;
}

static int temp_sync(struct fs_file_t *zfp)
{
	if (!zfp) {
		return -EINVAL;
	}

	if (nospace) {
		return -ENOSPC;
	}

	return 0;
}

static int temp_mkdir(struct fs_mount_t *mountp, const char *path)
{
	if (mountp == NULL || path == NULL) {
		return -EINVAL;
	}

	if (strcmp(mountp->mnt_point, path) == 0) {
		return -EPERM;
	}
	return 0;
}

static int temp_opendir(struct fs_dir_t *zdp, const char *path)
{
	if (zdp == NULL || path == NULL) {
		return -EINVAL;
	}

	if (zdp->dirp) {
		if (strcmp(zdp->dirp, path) == 0) {
			return -EIO;
		}
	}
	zdp->dirp = (char *)path;
	return 0;
}

static int i;
static int temp_readdir(struct fs_dir_t *zdp, struct fs_dirent *entry)
{
	if (!zdp) {
		return -EINVAL;
	}

	if (!entry) {
		return -ENOENT;
	}

	switch (i) {
	case 0:
		strcpy(entry->name, ".");
		entry->type = FS_DIR_ENTRY_DIR;
		i++;
		break;
	case 1:
		strcpy(entry->name, "testdir");
		entry->type = FS_DIR_ENTRY_DIR;
		i++;
		break;
	case 2:
		strcpy(entry->name, "test.txt");
		entry->type = FS_DIR_ENTRY_FILE;
		i++;
		break;
	default:
		strcpy(entry->name, "\0");
		i = 0;
		break;
	}
	return 0;
}

static int temp_closedir(struct fs_dir_t *zdp)
{
	if (!zdp) {
		return -EINVAL;
	}

	if (!(zdp->dirp)) {
		return -EIO;
	}
	zdp->dirp = NULL;
	return 0;
}

static int temp_stat(struct fs_mount_t *mountp,
		      const char *path, struct fs_dirent *entry)
{
	if (mountp == NULL || path == NULL || entry == NULL) {
		return -EINVAL;
	}

	return 0;
}

static int temp_statvfs(struct fs_mount_t *mountp,
			 const char *path, struct fs_statvfs *stat)
{
	if (mountp == NULL || path == NULL || stat == NULL) {
		return -EINVAL;
	}

	memset(stat, 0, sizeof(struct fs_statvfs));
	stat->f_bsize = 512;
	return 0;
}

static int temp_mount(struct fs_mount_t *mountp)
{
	if (mountp == NULL) {
		return -EINVAL;
	}

	if (mountp->mnt_point[mountp->mountp_len - 1] != ':') {
		return -EINVAL;
	}
	mp[mountp->type] = mountp;
	return 0;
}

static int temp_unmount(struct fs_mount_t *mountp)
{
	if (mountp == NULL) {
		return -EINVAL;
	}

	if (mp[mountp->type] == NULL) {
		return -EINVAL;
	}
	mp[mountp->type] = NULL;
	return 0;
}

/* File system interface */
struct fs_file_system_t temp_fs = {
	.open = temp_open,
	.close = temp_close,
	.read = temp_read,
	.write = temp_write,
	.lseek = temp_seek,
	.tell = temp_tell,
	.truncate = temp_truncate,
	.sync = temp_sync,
	.opendir = temp_opendir,
	.readdir = temp_readdir,
	.closedir = temp_closedir,
	.mount = temp_mount,
	.unmount = temp_unmount,
	.unlink = temp_unlink,
	.rename = temp_rename,
	.mkdir = temp_mkdir,
	.stat = temp_stat,
	.statvfs = temp_statvfs,
};
