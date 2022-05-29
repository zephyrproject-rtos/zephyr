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
#include <zephyr/posix/sys/ioctl_calls.h>
#include <zephyr/posix/unistd_calls.h>
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

#if defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
static const struct fd_op_vtable stdinout_fd_op_vtable;
#endif

static struct fd_entry fdtable[CONFIG_POSIX_MAX_FDS] = {
#if defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
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
	{
	0
	},
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

#ifdef CONFIG_USERSPACE
	if (z_is_in_user_syscall()) {
		struct z_object *zo;

		zo = z_object_find(entry->obj);
		if (zo != NULL && zo->type == K_OBJ_NET_SOCKET) {
			int res = z_object_validate(zo, K_OBJ_NET_SOCKET, _OBJ_INIT_TRUE);

			if (res != 0) {
				z_dump_object_error(res, entry->obj, zo, K_OBJ_NET_SOCKET);
				/* Invalidate the context, the caller doesn't have
				 * sufficient permission or there was some other
				 * problem with the net socket object
				 */
				return NULL;
			}
		}
	}
#endif /* CONFIG_USERSPACE */

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

#if (defined(CONFIG_POSIX_API) || \
	defined(CONFIG_NET_SOCKETS_POSIX_NAMES))
ssize_t z_impl_zephyr_read(int fd, void *buf, size_t sz)
{
	void *ctx;
	ssize_t res;
	struct k_mutex *lock;
	const struct fd_op_vtable *vtable;

	ctx = z_get_fd_obj_and_vtable(fd, &vtable, &lock);
	if (ctx == NULL) {
		return -1;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	res = vtable->read(ctx, buf, sz);

	k_mutex_unlock(lock);

	return res;
}

#ifdef CONFIG_USERSPACE
static inline ssize_t z_vrfy_zephyr_read(int fd, void *buf, size_t sz)
{
	Z_SYSCALL_MEMORY_WRITE(buf, sz);
	return z_impl_zephyr_read(fd, buf, sz);
}
#include <syscalls/zephyr_read_mrsh.c>
#endif

ssize_t read(int fd, void *buf, size_t sz)
{
	return zephyr_read(fd, buf, sz);
}
FUNC_ALIAS(read, _read, ssize_t);

ssize_t z_impl_zephyr_write(int fd, const void *buf, size_t sz)
{
	void *ctx;
	ssize_t res;
	struct k_mutex *lock;
	const struct fd_op_vtable *vtable;

	ctx = z_get_fd_obj_and_vtable(fd, &vtable, &lock);
	if (ctx == NULL) {
		return -1;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	res = vtable->write(ctx, buf, sz);

	k_mutex_unlock(lock);

	return res;
}

#ifdef CONFIG_USERSPACE
static inline ssize_t z_vrfy_zephyr_write(int fd, const void *buf, size_t sz)
{
	Z_SYSCALL_MEMORY_READ(buf, sz);
	return z_impl_zephyr_write(fd, buf, sz);
}
#include <syscalls/zephyr_write_mrsh.c>
#endif

ssize_t write(int fd, const void *buf, size_t sz)
{
	return zephyr_write(fd, buf, sz);
}
FUNC_ALIAS(write, _write, ssize_t);

int z_impl_zephyr_fsync(int fd)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return z_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, ZFD_IOCTL_FSYNC);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_fsync(int fd)
{
	return z_impl_zephyr_fsync(fd);
}
#include <syscalls/zephyr_fsync_mrsh.c>
#endif

int fsync(int fd)
{
	return zephyr_fsync(fd);
}

off_t z_impl_zephyr_lseek(int fd, off_t offset, int whence)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return z_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, ZFD_IOCTL_LSEEK,
			  offset, whence);
}

#ifdef CONFIG_USERSPACE
static inline off_t z_vrfy_zephyr_lseek(int fd, off_t offset, int whence)
{
	return z_impl_zephyr_lseek(fd, offset, whence);
}
#include <syscalls/zephyr_lseek_mrsh.c>
#endif

off_t lseek(int fd, off_t offset, int whence)
{
	return zephyr_lseek(fd, offset, whence);
}
#ifndef CONFIG_ARCH_POSIX
FUNC_ALIAS(lseek, _lseek, off_t);
#endif

int z_impl_zephyr_ioctl(int fd, unsigned long request, va_list args)
{
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, request, args);

	return res;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_ioctl(int fd, unsigned long request, va_list args)
{
	return z_impl_zephyr_ioctl(fd, request, args);
}
#include <syscalls/zephyr_ioctl_mrsh.c>
#endif

int ioctl(int fd, unsigned long request, ...)
{
	int res;
	va_list args;

	va_start(args, request);
	res = zephyr_ioctl(fd, request, args);
	va_end(args);

	return res;
}

int z_impl_zephyr_fcntl(int fd, int cmd, va_list args)
{
	void *ctx;
	ssize_t res;
	struct k_mutex *lock;
	const struct fd_op_vtable *vtable;

	/* Handle fdtable commands. */
	if (cmd == F_DUPFD) {
		/* Not implemented so far. */
		errno = EINVAL;
		return -1;
	}

	ctx = z_get_fd_obj_and_vtable(fd, &vtable, &lock);
	if (ctx == NULL) {
		return -1;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	/* The rest of commands are per-fd, handled by ioctl vmethod. */
	res = vtable->ioctl(ctx, cmd, args);

	k_mutex_unlock(lock);

	return res;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_fcntl(int sock, int cmd, va_list args)
{
	return z_impl_zephyr_fcntl(sock, cmd, args);
}
#include <syscalls/zephyr_fcntl_mrsh.c>
#endif

int fcntl(int fd, int cmd, ...)
{
	int res;
	va_list args;

	va_start(args, cmd);
	res = zephyr_fcntl(fd, cmd, args);
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

#endif /* defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES) */

/*
 * Must implement close() on CONFIG_ARCH_POSIX, otherwise we are unable to
 * properly run tests in tests/net/socket/poll. The reason, is that we
 * no longer have a wrapper pointing close() to zsock_close() but we still
 * desire that behaviour. Without the wrapper, we link in close() from the
 * system libc, which results in -1, errno == EBADF.
 */
#if defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES) ||                        \
	defined(CONFIG_ARCH_POSIX)
int z_impl_zephyr_close(int fd)
{
	void *ctx;
	ssize_t res;
	struct k_mutex *lock;
	const struct fd_op_vtable *vtable;

	ctx = z_get_fd_obj_and_vtable(fd, &vtable, &lock);
	if (ctx == NULL) {
		return -1;
	}

	(void)k_mutex_lock(lock, K_FOREVER);

	res = vtable->close(ctx);

	k_mutex_unlock(lock);

	z_free_fd(fd);

	return res;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_zephyr_close(int fd)
{
	return z_impl_zephyr_close(fd);
}
#include <syscalls/zephyr_close_mrsh.c>
#endif

int close(int fd)
{
	return zephyr_close(fd);
}
FUNC_ALIAS(close, _close, int);

#endif
/* defined(CONFIG_POSIX_API) || defined(CONFIG_NET_SOCKETS_POSIX_NAMES) ||
 * defined(CONFIG_ARCH_POSIX)
 */
