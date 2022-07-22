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
#include <fcntl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/speculation.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/sys/atomic.h>

struct fd_entry {
	void *obj;
	const struct fd_op_vtable *vtable;
	atomic_t refcount;
	struct k_mutex lock;
};

#ifdef CONFIG_POSIX_API
static const struct fd_op_vtable stdinout_fd_op_vtable;
#endif

static struct fd_entry fdtable[CONFIG_POSIX_MAX_FDS] = {
#ifdef CONFIG_POSIX_API
	/*
	 * Predefine entries for stdin/stdout/stderr.
	 */
	{
		/* STDIN */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1)
	},
	{
		/* STDOUT */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1)
	},
	{
		/* STDERR */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1)
	},
#else
	0
#endif
};

static K_MUTEX_DEFINE(fdtable_lock);

static int z_fd_ref(int fd)
{
	return atomic_inc(&fdtable[fd].refcount) + 1;
}

static int z_fd_unref(int fd)
{
	atomic_val_t old_rc;

	/* Reference counter must be checked to avoid decrement refcount below
	 * zero causing file descriptor leak. Loop statement below executes
	 * atomic decrement if refcount value is grater than zero. Otherwise,
	 * refcount is not going to be written.
	 */
	do {
		old_rc = atomic_get(&fdtable[fd].refcount);
		if (!old_rc) {
			return 0;
		}
	} while (!atomic_cas(&fdtable[fd].refcount, old_rc, old_rc - 1));

	if (old_rc != 1) {
		return old_rc - 1;
	}

	fdtable[fd].obj = NULL;
	fdtable[fd].vtable = NULL;

	return 0;
}

static int _find_fd_entry(void)
{
	int fd;

	for (fd = 0; fd < ARRAY_SIZE(fdtable); fd++) {
		if (!atomic_get(&fdtable[fd].refcount)) {
			return fd;
		}
	}

	errno = ENFILE;
	return -1;
}

static int _check_fd(int fd)
{
	if (fd < 0 || fd >= ARRAY_SIZE(fdtable)) {
		errno = EBADF;
		return -1;
	}

	fd = k_array_index_sanitize(fd, ARRAY_SIZE(fdtable));

	if (!atomic_get(&fdtable[fd].refcount)) {
		errno = EBADF;
		return -1;
	}

	return 0;
}

void *z_get_fd_obj(int fd, const struct fd_op_vtable *vtable, int err)
{
	struct fd_entry *entry;

	if (_check_fd(fd) < 0) {
		return NULL;
	}

	entry = &fdtable[fd];

	if (vtable != NULL && entry->vtable != vtable) {
		errno = err;
		return NULL;
	}

	return entry->obj;
}

void *z_get_fd_obj_and_vtable(int fd, const struct fd_op_vtable **vtable,
			      struct k_mutex **lock)
{
	struct fd_entry *entry;

	if (_check_fd(fd) < 0) {
		return NULL;
	}

	entry = &fdtable[fd];
	*vtable = entry->vtable;

	if (lock) {
		*lock = &entry->lock;
	}

	return entry->obj;
}

int z_reserve_fd(void)
{
	int fd;

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	fd = _find_fd_entry();
	if (fd >= 0) {
		/* Mark entry as used, z_finalize_fd() will fill it in. */
		(void)z_fd_ref(fd);
		fdtable[fd].obj = NULL;
		fdtable[fd].vtable = NULL;
		k_mutex_init(&fdtable[fd].lock);
	}

	k_mutex_unlock(&fdtable_lock);

	return fd;
}

void z_finalize_fd(int fd, void *obj, const struct fd_op_vtable *vtable)
{
	/* Assumes fd was already bounds-checked. */
#ifdef CONFIG_USERSPACE
	/* descriptor context objects are inserted into the table when they
	 * are ready for use. Mark the object as initialized and grant the
	 * caller (and only the caller) access.
	 *
	 * This call is a no-op if obj is invalid or points to something
	 * not a kernel object.
	 */
	z_object_recycle(obj);
#endif
	fdtable[fd].obj = obj;
	fdtable[fd].vtable = vtable;

	/* Let the object know about the lock just in case it needs it
	 * for something. For BSD sockets, the lock is used with condition
	 * variables to avoid keeping the lock for a long period of time.
	 */
	if (vtable && vtable->ioctl) {
		(void)z_fdtable_call_ioctl(vtable, obj, ZFD_IOCTL_SET_LOCK,
					   &fdtable[fd].lock);
	}
}

void z_free_fd(int fd)
{
	/* Assumes fd was already bounds-checked. */
	(void)z_fd_unref(fd);
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
	ssize_t res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	(void)k_mutex_lock(&fdtable[fd].lock, K_FOREVER);

	res = fdtable[fd].vtable->read(fdtable[fd].obj, buf, sz);

	k_mutex_unlock(&fdtable[fd].lock);

	return res;
}
FUNC_ALIAS(read, _read, ssize_t);

ssize_t write(int fd, const void *buf, size_t sz)
{
	ssize_t res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	(void)k_mutex_lock(&fdtable[fd].lock, K_FOREVER);

	res = fdtable[fd].vtable->write(fdtable[fd].obj, buf, sz);

	k_mutex_unlock(&fdtable[fd].lock);

	return res;
}
FUNC_ALIAS(write, _write, ssize_t);

int close(int fd)
{
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	(void)k_mutex_lock(&fdtable[fd].lock, K_FOREVER);

	res = fdtable[fd].vtable->close(fdtable[fd].obj);

	k_mutex_unlock(&fdtable[fd].lock);

	z_free_fd(fd);

	return res;
}
FUNC_ALIAS(close, _close, int);

int fsync(int fd)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return z_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, ZFD_IOCTL_FSYNC);
}

off_t lseek(int fd, off_t offset, int whence)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return z_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, ZFD_IOCTL_LSEEK,
			  offset, whence);
}
FUNC_ALIAS(lseek, _lseek, off_t);

int ioctl(int fd, unsigned long request, ...)
{
	va_list args;
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	va_start(args, request);
	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, request, args);
	va_end(args);

	return res;
}

int fcntl(int fd, int cmd, ...)
{
	va_list args;
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	/* Handle fdtable commands. */
	if (cmd == F_DUPFD) {
		/* Not implemented so far. */
		errno = EINVAL;
		return -1;
	}

	/* The rest of commands are per-fd, handled by ioctl vmethod. */
	va_start(args, cmd);
	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, cmd, args);
	va_end(args);

	return res;
}

/*
 * fd operations for stdio/stdout/stderr
 */

int z_impl_zephyr_write_stdout(const char *buf, int nbytes);

static ssize_t stdinout_read_vmeth(void *obj, void *buffer, size_t count)
{
	return 0;
}

static ssize_t stdinout_write_vmeth(void *obj, const void *buffer, size_t count)
{
#if defined(CONFIG_BOARD_NATIVE_POSIX)
	return write(1, buffer, count);
#elif defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_ARCMWDT_LIBC)
	return z_impl_zephyr_write_stdout(buffer, count);
#else
	return 0;
#endif
}

static int stdinout_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	errno = EINVAL;
	return -1;
}


static const struct fd_op_vtable stdinout_fd_op_vtable = {
	.read = stdinout_read_vmeth,
	.write = stdinout_write_vmeth,
	.ioctl = stdinout_ioctl_vmeth,
};

#endif /* CONFIG_POSIX_API */
