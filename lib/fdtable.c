/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File descriptor table
 *
 * This file provides generic file descriptor table implementation, suitable
 * for any I/O object implementing POSIX I/O semantics (i.e. read/write +
 * aux operations).
 */

#include <errno.h>
#include <kernel.h>
#include <misc/fdtable.h>

struct fd_entry {
	void *obj;
	const struct fd_op_vtable *vtable;
};

static struct fd_entry fdtable[CONFIG_POSIX_MAX_FDS];

static K_MUTEX_DEFINE(fdtable_lock);

static int _find_fd_entry(void)
{
	int fd;

	for (fd = 0; fd < ARRAY_SIZE(fdtable); fd++) {
		if (fdtable[fd].obj == NULL) {
			return fd;
		}
	}

	errno = ENFILE;
	return -1;
}

static int _check_fd(int fd)
{
	if (fd < 0 || fd >= ARRAY_SIZE(fdtable) || fdtable[fd].obj == NULL) {
		errno = EBADF;
		return -1;
	}

	return 0;
}

void *z_get_fd_obj(int fd, const struct fd_op_vtable *vtable, int err)
{
	struct fd_entry *fd_entry;

	if (_check_fd(fd) < 0) {
		return NULL;
	}

	fd_entry = &fdtable[fd];

	if (vtable != NULL && fd_entry->vtable != vtable) {
		errno = err;
		return NULL;
	}

	return fd_entry->obj;
}

int z_reserve_fd(void)
{
	int fd;

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	fd = _find_fd_entry();
	if (fd >= 0) {
		/* Mark entry as used, z_finalize_fd() will fill it in. */
		fdtable[fd].obj = FD_OBJ_RESERVED;
	}

	k_mutex_unlock(&fdtable_lock);

	return fd;
}

void z_finalize_fd(int fd, void *obj, const struct fd_op_vtable *vtable)
{
	/* Assumes fd was already bounds-checked. */
	fdtable[fd].obj = obj;
	fdtable[fd].vtable = vtable;
}

void z_free_fd(int fd)
{
	/* Assumes fd was already bounds-checked. */
	fdtable[fd].obj = NULL;
}

int z_alloc_fd(void *obj, const struct fd_op_vtable *vtable)
{
	int fd;

	fd = z_reserve_fd();
	if (fd >= 0) {
		z_finalize_fd(fd, obj, vtable);
	}

	return fd;
}

#ifdef CONFIG_POSIX_API

ssize_t read(int fd, void *buf, size_t sz)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return fdtable[fd].vtable->read(fdtable[fd].obj, buf, sz);
}

ssize_t write(int fd, const void *buf, size_t sz)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return fdtable[fd].vtable->write(fdtable[fd].obj, buf, sz);
}

int close(int fd)
{
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, ZFD_IOCTL_CLOSE);
	z_free_fd(fd);

	return res;
}

int fsync(int fd)
{
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, ZFD_IOCTL_FSYNC);
	z_free_fd(fd);

	return res;
}

off_t lseek(int fd, off_t offset, int whence)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return fdtable[fd].vtable->ioctl(fdtable[fd].obj, ZFD_IOCTL_LSEEK,
					 offset, whence);
}

#endif /* CONFIG_POSIX_API */
