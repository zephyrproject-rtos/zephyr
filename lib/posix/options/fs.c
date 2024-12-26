/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "fs_priv.h"

#include <errno.h>
#include <zephyr/kernel.h>
#include <limits.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/dirent.h>
#include <string.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/posix/sys/stat.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/fs/fs.h>

int zvfs_fstat(int fd, struct stat *buf);

BUILD_ASSERT(PATH_MAX >= MAX_FILE_NAME, "PATH_MAX is less than MAX_FILE_NAME");

static struct posix_fs_desc desc_array[CONFIG_POSIX_OPEN_MAX];

static struct fs_dirent fdirent;
static struct dirent pdirent;

static struct fd_op_vtable fs_fd_op_vtable;

static struct posix_fs_desc *posix_fs_alloc_obj(bool is_dir)
{
	int i;
	struct posix_fs_desc *ptr = NULL;
	unsigned int key = irq_lock();

	for (i = 0; i < CONFIG_POSIX_OPEN_MAX; i++) {
		if (desc_array[i].used == false) {
			ptr = &desc_array[i];
			ptr->used = true;
			ptr->is_dir = is_dir;
			break;
		}
	}
	irq_unlock(key);

	return ptr;
}

static inline void posix_fs_free_obj(struct posix_fs_desc *ptr)
{
	ptr->used = false;
}

static int posix_mode_to_zephyr(int mf)
{
	int mode = (mf & O_CREAT) ? FS_O_CREATE : 0;

	mode |= (mf & O_APPEND) ? FS_O_APPEND : 0;
	mode |= (mf & O_TRUNC) ? FS_O_TRUNC : 0;

	switch (mf & O_ACCMODE) {
	case O_RDONLY:
		mode |= FS_O_READ;
		break;
	case O_WRONLY:
		mode |= FS_O_WRITE;
		break;
	case O_RDWR:
		mode |= FS_O_RDWR;
		break;
	default:
		break;
	}

	return mode;
}

int zvfs_open(const char *name, int flags, int mode)
{
	int rc, fd;
	struct posix_fs_desc *ptr = NULL;
	int zmode = posix_mode_to_zephyr(flags);

	if (zmode < 0) {
		return zmode;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	ptr = posix_fs_alloc_obj(false);
	if (ptr == NULL) {
		rc = -EMFILE;
		goto out_err;
	}

	fs_file_t_init(&ptr->file);

	if (flags & O_CREAT) {
		flags &= ~O_CREAT;

		rc = fs_open(&ptr->file, name, FS_O_CREATE | (mode & O_ACCMODE));
		if (rc < 0) {
			goto out_err;
		}
		rc = fs_close(&ptr->file);
		if (rc < 0) {
			goto out_err;
		}
	}

	rc = fs_open(&ptr->file, name, zmode);
	if (rc < 0) {
		goto out_err;
	}

	zvfs_finalize_fd(fd, ptr, &fs_fd_op_vtable);

	goto out;

out_err:
	if (ptr != NULL) {
		posix_fs_free_obj(ptr);
	}

	zvfs_free_fd(fd);
	errno = -rc;
	return -1;

out:
	return fd;
}

static int fs_close_vmeth(void *obj)
{
	struct posix_fs_desc *ptr = obj;
	int rc;

	rc = fs_close(&ptr->file);
	posix_fs_free_obj(ptr);

	return rc;
}

static int fs_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	int rc = 0;
	struct posix_fs_desc *ptr = obj;

	switch (request) {
	case ZFD_IOCTL_FSYNC: {
		rc = fs_sync(&ptr->file);
		break;
	}
	case ZFD_IOCTL_LSEEK: {
		off_t offset;
		int whence;

		offset = va_arg(args, off_t);
		whence = va_arg(args, int);

		rc = fs_seek(&ptr->file, offset, whence);
		if (rc == 0) {
			rc = fs_tell(&ptr->file);
		}
		break;
	}
	case ZFD_IOCTL_TRUNCATE: {
		off_t length;

		length = va_arg(args, off_t);

		rc = fs_truncate(&ptr->file, length);
		if (rc < 0) {
			errno = -rc;
			return -1;
		}
		break;
	}
	default:
		errno = EOPNOTSUPP;
		return -1;
	}

	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}

/**
 * @brief Write to a file.
 *
 * See IEEE 1003.1
 */
static ssize_t fs_write_vmeth(void *obj, const void *buffer, size_t count)
{
	ssize_t rc;
	struct posix_fs_desc *ptr = obj;

	rc = fs_write(&ptr->file, buffer, count);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}

/**
 * @brief Read from a file.
 *
 * See IEEE 1003.1
 */
static ssize_t fs_read_vmeth(void *obj, void *buffer, size_t count)
{
	ssize_t rc;
	struct posix_fs_desc *ptr = obj;

	rc = fs_read(&ptr->file, buffer, count);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}

static struct fd_op_vtable fs_fd_op_vtable = {
	.read = fs_read_vmeth,
	.write = fs_write_vmeth,
	.close = fs_close_vmeth,
	.ioctl = fs_ioctl_vmeth,
};

/**
 * @brief Open a directory stream.
 *
 * See IEEE 1003.1
 */
DIR *opendir(const char *dirname)
{
	int rc;
	struct posix_fs_desc *ptr;

	ptr = posix_fs_alloc_obj(true);
	if (ptr == NULL) {
		errno = EMFILE;
		return NULL;
	}

	fs_dir_t_init(&ptr->dir);

	rc = fs_opendir(&ptr->dir, dirname);
	if (rc < 0) {
		posix_fs_free_obj(ptr);
		errno = -rc;
		return NULL;
	}

	return ptr;
}

/**
 * @brief Close a directory stream.
 *
 * See IEEE 1003.1
 */
int closedir(DIR *dirp)
{
	int rc;
	struct posix_fs_desc *ptr = dirp;

	if (dirp == NULL) {
		errno = EBADF;
		return -1;
	}

	rc = fs_closedir(&ptr->dir);

	posix_fs_free_obj(ptr);

	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return 0;
}

/**
 * @brief Read a directory.
 *
 * See IEEE 1003.1
 */
struct dirent *readdir(DIR *dirp)
{
	int rc;
	struct posix_fs_desc *ptr = dirp;

	if (dirp == NULL) {
		errno = EBADF;
		return NULL;
	}

	rc = fs_readdir(&ptr->dir, &fdirent);
	if (rc < 0) {
		errno = -rc;
		return NULL;
	}

	if (fdirent.name[0] == 0) {
		/* assume end-of-dir, leave errno untouched */
		return NULL;
	}

	rc = strlen(fdirent.name);
	rc = (rc < MAX_FILE_NAME) ? rc : (MAX_FILE_NAME - 1);
	(void)memcpy(pdirent.d_name, fdirent.name, rc);

	/* Make sure the name is NULL terminated */
	pdirent.d_name[rc] = '\0';
	return &pdirent;
}

/**
 * @brief Rename a file.
 *
 * See IEEE 1003.1
 */
int rename(const char *old, const char *new)
{
	int rc;

	rc = fs_rename(old, new);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return 0;
}

/**
 * @brief Remove a directory entry.
 *
 * See IEEE 1003.1
 */
int unlink(const char *path)
{
	int rc;

	rc = fs_unlink(path);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}
	return 0;
}

/**
 * @brief Get file status.
 *
 * See IEEE 1003.1
 */
int stat(const char *path, struct stat *buf)
{
	int rc;
	struct fs_statvfs stat_vfs;
	struct fs_dirent stat_file;

	if (buf == NULL) {
		errno = EBADF;
		return -1;
	}

	rc = fs_statvfs(path, &stat_vfs);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	rc = fs_stat(path, &stat_file);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	memset(buf, 0, sizeof(struct stat));

	switch (stat_file.type) {
	case FS_DIR_ENTRY_FILE:
		buf->st_mode = S_IFREG;
		break;
	case FS_DIR_ENTRY_DIR:
		buf->st_mode = S_IFDIR;
		break;
	default:
		errno = EIO;
		return -1;
	}
	buf->st_size = stat_file.size;
	buf->st_blksize = stat_vfs.f_bsize;
	/*
	 * This is a best effort guess, as this information is not provided
	 * by the fs_stat function.
	 */
	buf->st_blocks = (stat_file.size + stat_vfs.f_bsize - 1) / stat_vfs.f_bsize;

	return 0;
}

/**
 * @brief Make a directory.
 *
 * See IEEE 1003.1
 */
int mkdir(const char *path, mode_t mode)
{
	int rc;

	ARG_UNUSED(mode);

	rc = fs_mkdir(path);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return 0;
}

int fstat(int fildes, struct stat *buf)
{
	return zvfs_fstat(fildes, buf);
}
#ifdef CONFIG_POSIX_FILE_SYSTEM_ALIAS_FSTAT
FUNC_ALIAS(fstat, _fstat, int);
#endif

/**
 * @brief Remove a directory.
 *
 * See IEEE 1003.1
 */
int rmdir(const char *path)
{
	return unlink(path);
}
