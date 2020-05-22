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
#include <kernel.h>
#include <sys/fdtable.h>
#include <sys/speculation.h>
#ifdef CONFIG_POSIX_API
#include <posix/unistd.h>
#include <posix/sys/ioctl.h>
#endif
#include <syscall_handler.h>

/* Number of arguments which can be passed to public ioctl calls
 * (i.e. from userspace to kernel space).
 * Arbitrary value is supported (at the expense of stack usage). Can
 * be increased when ioctl's with more arguments are added.
 * Note that kernelspace-kernelspace ioctl calls are handled
 * differently (in z_fdtable_call_ioctl()).
 */
#define MAX_USERSPACE_IOCTL_ARGS 1

struct fd_entry {
	void *obj;
	const struct fd_op_vtable *vtable;
};

/* A few magic values for fd_entry::obj used in the code. */
#define FD_OBJ_RESERVED (void *)1
#define FD_OBJ_STDIN  (void *)0x10
#define FD_OBJ_STDOUT (void *)0x11
#define FD_OBJ_STDERR (void *)0x12

#ifdef CONFIG_POSIX_API
static const struct fd_op_vtable stdinout_fd_op_vtable;
#endif

static struct fd_entry fdtable[CONFIG_POSIX_MAX_FDS] = {
#ifdef CONFIG_POSIX_API
	/*
	 * Predefine entries for stdin/stdout/stderr. Object pointer
	 * is unused and just should be !0 (random different values
	 * are used to posisbly help with debugging).
	 */
	{FD_OBJ_STDIN,  &stdinout_fd_op_vtable},
	{FD_OBJ_STDOUT, &stdinout_fd_op_vtable},
	{FD_OBJ_STDERR, &stdinout_fd_op_vtable},
#endif
};

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
	if (fd < 0 || fd >= ARRAY_SIZE(fdtable)) {
		errno = EBADF;
		return -1;
	}

	fd = k_array_index_sanitize(fd, ARRAY_SIZE(fdtable));

	if (fdtable[fd].obj == NULL) {
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

void *z_get_fd_obj_and_vtable(int fd, const struct fd_op_vtable **vtable)
{
	struct fd_entry *fd_entry;

	if (_check_fd(fd) < 0) {
		return NULL;
	}

	fd_entry = &fdtable[fd];
	*vtable = fd_entry->vtable;

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

ssize_t z_impl_sys_read(int fd, void *buf, size_t sz)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return fdtable[fd].vtable->read(fdtable[fd].obj, buf, sz);
}

#ifdef CONFIG_USERSPACE
ssize_t z_vrfy_sys_read(int fd, void *buf, size_t sz)
{
	if (Z_SYSCALL_MEMORY_WRITE(buf, sz)) {
		errno = EFAULT;
		return -1;
	}

	return z_impl_sys_read(fd, buf, sz);
}
#include <syscalls/sys_read_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* Normal C function wrapping a corresponding syscall. Required to ensure
 * classic C linkage.
 */
ssize_t read(int fd, void *buf, size_t sz)
{
	return sys_read(fd, buf, sz);
}
FUNC_ALIAS(read, _read, ssize_t);

ssize_t z_impl_sys_write(int fd, const void *buf, size_t sz)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return fdtable[fd].vtable->write(fdtable[fd].obj, buf, sz);
}

#ifdef CONFIG_USERSPACE
ssize_t z_vrfy_sys_write(int fd, const void *buf, size_t sz)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(buf, sz));

	return z_impl_sys_write(fd, buf, sz);
}
#include <syscalls/sys_write_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* Normal C function wrapping a corresponding syscall. Required to ensure
 * classic C linkage.
 */
ssize_t write(int fd, const void *buf, size_t sz)
{
	return sys_write(fd, buf, sz);
}
FUNC_ALIAS(write, _write, ssize_t);

int z_impl_sys_close(int fd)
{
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	res = z_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, ZFD_IOCTL_CLOSE, 0);
	z_free_fd(fd);

	return res;
}

#ifdef CONFIG_USERSPACE
ssize_t z_vrfy_sys_close(int fd)
{
	return z_impl_sys_close(fd);
}
#include <syscalls/sys_close_mrsh.c>
#endif /* CONFIG_USERSPACE */

int close(int fd)
{
	return sys_close(fd);
}
FUNC_ALIAS(close, _close, int);

int fsync(int fd)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return z_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj,
				    ZFD_IOCTL_FSYNC, 0);
}

off_t lseek(int fd, off_t offset, int whence)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return z_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj,
				    ZFD_IOCTL_LSEEK,
				    2, offset, whence);
}
FUNC_ALIAS(lseek, _lseek, off_t);

int z_impl_sys_ioctl(int fd, unsigned long request, long n_args, uintptr_t *args)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return fdtable[fd].vtable->ioctl(fdtable[fd].obj, request, n_args, args);
}

#ifdef CONFIG_USERSPACE
ssize_t z_vrfy_sys_ioctl(int fd, unsigned long request, long n_args, uintptr_t *args)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(args, sizeof(*args) * n_args));

	if (request >= ZFD_IOCTL_PRIVATE) {
		errno = EINVAL;
		return -1;
	}

	return z_impl_sys_ioctl(fd, request, n_args, args);
}
#include <syscalls/sys_ioctl_mrsh.c>
#endif /* CONFIG_USERSPACE */

static int _vioctl(int fd, unsigned long request, va_list args)
{
	int i, n_args;
	/* We assume that for argument passing [on stack], natural word size
	 * of the plaform is used. So for example, for LP64 platform, where
	 * int is 32-bit, it's still pushed as 64-bit value on stack.
	 */
	uintptr_t marshalled_args[MAX_USERSPACE_IOCTL_ARGS];

	/* Calculate number of arguments for individual ioctl requests. */
	switch (request) {
	case F_GETFL:
		n_args = 0;
		break;
	case F_SETFL:
		n_args = 1;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (n_args > ARRAY_SIZE(marshalled_args)) {
		/* Use distinguishable error code. */
		errno = EDOM;
		return -1;
	}

	for (i = 0; i < n_args; i++) {
		marshalled_args[i] = va_arg(args, uintptr_t);
	}

	return sys_ioctl(fd, request, n_args, marshalled_args);
}

int ioctl(int fd, unsigned long request, ...)
{
	va_list args;
	int res;

	va_start(args, request);
	res = _vioctl(fd, request, args);
	va_end(args);

	return res;
}

/*
 * In the SimpleLink case, we have yet to add support for the fdtable
 * feature. The socket offload subsys has already defined fcntl, hence we
 * avoid redefining fcntl here.
 */
#ifndef CONFIG_SOC_FAMILY_TISIMPLELINK
int fcntl(int fd, int cmd, ...)
{
	va_list args;
	int res;

	/* Handle fdtable commands. */
	switch (cmd) {
	case F_DUPFD:
		/* Not implemented so far. */
		errno = EINVAL;
		return -1;
	}

	/* The rest of commands are per-fd, handled by ioctl vmethod. */
	va_start(args, cmd);
	res = _vioctl(fd, cmd, args);
	va_end(args);

	return res;
}
#endif

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
#elif defined(CONFIG_NEWLIB_LIBC)
	return z_impl_zephyr_write_stdout(buffer, count);
#else
	return 0;
#endif
}

static int stdinout_ioctl_vmeth(void *obj, unsigned long request,
				long n_args, uintptr_t *args)
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
