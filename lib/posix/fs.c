/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel.h>
#include <limits.h>
#include <posix/pthread.h>
#include <posix/unistd.h>
#include <string.h>

BUILD_ASSERT_MSG(PATH_MAX > MAX_FILE_NAME,
		"PATH_MAX is less than MAX_FILE_NAME");

union file_desc {
	struct fs_file_t file;
	struct fs_dir_t	dir;
};

struct posix_fs_desc {
	union file_desc desc;
	bool is_dir;
	bool used;
};

static struct posix_fs_desc desc_array[CONFIG_POSIX_MAX_OPEN_FILES];

static struct fs_dirent fdirent;
static struct dirent pdirent;

static int posix_fs_alloc_fd(union file_desc **ptr, bool is_dir)
{
	int fd;
	unsigned int key = irq_lock();

	for (fd = 0; fd < CONFIG_POSIX_MAX_OPEN_FILES; fd++) {
		if (desc_array[fd].used == false) {
			*ptr = &desc_array[fd].desc;
			desc_array[fd].used = true;
			desc_array[fd].is_dir = is_dir;
			break;
		}
	}
	irq_unlock(key);

	if (fd >= CONFIG_POSIX_MAX_OPEN_FILES) {
		return -1;
	}

	return fd;
}

static int posix_fs_get_ptr(int fd, union file_desc **ptr, bool is_dir)
{
	int rc = 0;
	unsigned int key;

	if (fd < 0 || fd >= CONFIG_POSIX_MAX_OPEN_FILES) {
		return -1;
	}

	key = irq_lock();

	if ((desc_array[fd].used == true) &&
		(desc_array[fd].is_dir == is_dir)) {
		*ptr = &desc_array[fd].desc;
	} else {
		rc = -1;
	}
	irq_unlock(key);

	return rc;
}

static inline void posix_fs_free_ptr(struct posix_fs_desc *ptr)
{
	struct posix_fs_desc *desc = ptr;
	unsigned int key = irq_lock();

	desc->used = false;
	desc->is_dir = false;
	irq_unlock(key);
}

static inline void posix_fs_free_fd(int fd)
{
	posix_fs_free_ptr(&desc_array[fd]);
}

/**
 * @brief Open a file.
 *
 * See IEEE 1003.1
 */
int open(const char *name, int flags)
{
	int rc, fd;
	struct fs_file_t *ptr = NULL;

	ARG_UNUSED(flags);

	fd = posix_fs_alloc_fd((union file_desc **)&ptr, false);
	if ((fd < 0) || (ptr == NULL)) {
		errno = ENFILE;
		return -1;
	}
	(void)memset(ptr, 0, sizeof(struct fs_file_t));

	rc = fs_open(ptr, name);
	if (rc < 0) {
		posix_fs_free_fd(fd);
		errno = -rc;
		return -1;
	}

	return fd;
}

/**
 * @brief Close a file descriptor.
 *
 * See IEEE 1003.1
 */
int close(int fd)
{
	int rc;
	struct fs_file_t *ptr = NULL;

	if (posix_fs_get_ptr(fd, (union file_desc **)&ptr, false)) {
		errno = EBADF;
		return -1;
	}

	rc = fs_close(ptr);

	/* Free file ptr memory */
	posix_fs_free_fd(fd);

	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return 0;
}

/**
 * @brief Write to a file.
 *
 * See IEEE 1003.1
 */
ssize_t write(int fd, char *buffer, unsigned int count)
{
	ssize_t rc;
	struct fs_file_t *ptr = NULL;

	if (posix_fs_get_ptr(fd, (union file_desc **)&ptr, false)) {
		errno = EBADF;
		return -1;
	}

	rc = fs_write(ptr, buffer, count);
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
ssize_t read(int fd, char *buffer, unsigned int count)
{
	ssize_t rc;
	struct fs_file_t *ptr = NULL;

	if (posix_fs_get_ptr(fd, (union file_desc **)&ptr, false)) {
		errno = EBADF;
		return -1;
	}

	rc = fs_read(ptr, buffer, count);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}

/**
 * @brief Move read/write file offset.
 *
 * See IEEE 1003.1
 */
int lseek(int fd, int offset, int whence)
{
	int rc;
	struct fs_file_t *ptr = NULL;

	if (posix_fs_get_ptr(fd, (union file_desc **)&ptr, false)) {
		errno = EBADF;
		return -1;
	}

	rc = fs_seek(ptr, offset, whence);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return 0;
}

/**
 * @brief Open a directory stream.
 *
 * See IEEE 1003.1
 */
DIR *opendir(const char *dirname)
{
	int rc, fd;
	struct fs_dir_t *ptr = NULL;

	fd = posix_fs_alloc_fd((union file_desc **)&ptr, true);
	if ((fd < 0) || (ptr == NULL)) {
		errno = EMFILE;
		return NULL;
	}
	(void)memset(ptr, 0, sizeof(struct fs_dir_t));

	rc = fs_opendir(ptr, dirname);
	if (rc < 0) {
		posix_fs_free_fd(fd);
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

	if (dirp == NULL) {
		errno = EBADF;
		return -1;
	}

	rc = fs_closedir(dirp);

	/* Free file ptr memory */
	posix_fs_free_ptr((struct posix_fs_desc *)dirp);

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

	if (dirp == NULL) {
		errno = EBADF;
		return NULL;
	}

	rc = fs_readdir(dirp, &fdirent);
	if (rc < 0) {
		errno = -rc;
		return NULL;
	}

	rc = strlen(fdirent.name);
	rc = (rc < PATH_MAX) ? rc : (PATH_MAX - 1);
	memcpy(pdirent.d_name, fdirent.name, rc);

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
	struct fs_statvfs stat;

	if (buf == NULL) {
		errno = EBADF;
		return -1;
	}

	rc = fs_statvfs(path, &stat);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	buf->st_size = stat.f_bsize * stat.f_blocks;
	buf->st_blksize = stat.f_bsize;
	buf->st_blocks = stat.f_blocks;
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
