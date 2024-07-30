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
#include <string.h>

#include <zephyr/posix/fcntl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/speculation.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/atomic.h>

struct stat;

struct fd_entry {
	void *obj;
	const struct fd_op_vtable *vtable;
	atomic_t refcount;
	struct k_mutex lock;
	struct k_condvar cond;
	size_t offset;
	uint32_t mode;
};

#if defined(CONFIG_POSIX_DEVICE_IO)
static const struct fd_op_vtable stdinout_fd_op_vtable;

BUILD_ASSERT(CONFIG_ZVFS_OPEN_MAX >= 3, "CONFIG_ZVFS_OPEN_MAX >= 3 for CONFIG_POSIX_DEVICE_IO");
#endif /* defined(CONFIG_POSIX_DEVICE_IO) */

static struct fd_entry fdtable[CONFIG_ZVFS_OPEN_MAX] = {
#if defined(CONFIG_POSIX_DEVICE_IO)
	/*
	 * Predefine entries for stdin/stdout/stderr.
	 */
	{
		/* STDIN */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[0].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[0].cond),
	},
	{
		/* STDOUT */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[1].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[1].cond),
	},
	{
		/* STDERR */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[2].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[2].cond),
	},
#else
	{0},
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
	if ((fd < 0) || (fd >= ARRAY_SIZE(fdtable))) {
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

#ifdef CONFIG_ZTEST
bool fdtable_fd_is_initialized(int fd)
{
	struct k_mutex ref_lock;
	struct k_condvar ref_cond;

	if (fd < 0 || fd >= ARRAY_SIZE(fdtable)) {
		return false;
	}

	ref_lock = (struct k_mutex)Z_MUTEX_INITIALIZER(fdtable[fd].lock);
	if (memcmp(&ref_lock, &fdtable[fd].lock, sizeof(ref_lock)) != 0) {
		return false;
	}

	ref_cond = (struct k_condvar)Z_CONDVAR_INITIALIZER(fdtable[fd].cond);
	if (memcmp(&ref_cond, &fdtable[fd].cond, sizeof(ref_cond)) != 0) {
		return false;
	}

	return true;
}
#endif /* CONFIG_ZTEST */

void *zvfs_get_fd_obj(int fd, const struct fd_op_vtable *vtable, int err)
{
	struct fd_entry *entry;

	if (_check_fd(fd) < 0) {
		return NULL;
	}

	entry = &fdtable[fd];

	if ((vtable != NULL) && (entry->vtable != vtable)) {
		errno = err;
		return NULL;
	}

	return entry->obj;
}

static int z_get_fd_by_obj_and_vtable(void *obj, const struct fd_op_vtable *vtable)
{
	int fd;

	for (fd = 0; fd < ARRAY_SIZE(fdtable); fd++) {
		if (fdtable[fd].obj == obj && fdtable[fd].vtable == vtable) {
			return fd;
		}
	}

	errno = ENFILE;
	return -1;
}

bool zvfs_get_obj_lock_and_cond(void *obj, const struct fd_op_vtable *vtable, struct k_mutex **lock,
			     struct k_condvar **cond)
{
	int fd;
	struct fd_entry *entry;

	fd = z_get_fd_by_obj_and_vtable(obj, vtable);
	if (_check_fd(fd) < 0) {
		return false;
	}

	entry = &fdtable[fd];

	if (lock) {
		*lock = &entry->lock;
	}

	if (cond) {
		*cond = &entry->cond;
	}

	return true;
}

void *zvfs_get_fd_obj_and_vtable(int fd, const struct fd_op_vtable **vtable,
			      struct k_mutex **lock)
{
	struct fd_entry *entry;

	if (_check_fd(fd) < 0) {
		return NULL;
	}

	entry = &fdtable[fd];
	*vtable = entry->vtable;

	if (lock != NULL) {
		*lock = &entry->lock;
	}

	return entry->obj;
}

int zvfs_reserve_fd(void)
{
	int fd;

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	fd = _find_fd_entry();
	if (fd >= 0) {
		/* Mark entry as used, zvfs_finalize_fd() will fill it in. */
		(void)z_fd_ref(fd);
		fdtable[fd].obj = NULL;
		fdtable[fd].vtable = NULL;
		k_mutex_init(&fdtable[fd].lock);
		k_condvar_init(&fdtable[fd].cond);
	}

	k_mutex_unlock(&fdtable_lock);

	return fd;
}

void zvfs_finalize_typed_fd(int fd, void *obj, const struct fd_op_vtable *vtable, uint32_t mode)
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
	k_object_recycle(obj);
#endif
	fdtable[fd].obj = obj;
	fdtable[fd].vtable = vtable;
	fdtable[fd].mode = mode;

	/* Let the object know about the lock just in case it needs it
	 * for something. For BSD sockets, the lock is used with condition
	 * variables to avoid keeping the lock for a long period of time.
	 */
	if (vtable && vtable->ioctl) {
		(void)zvfs_fdtable_call_ioctl(vtable, obj, ZFD_IOCTL_SET_LOCK,
					   &fdtable[fd].lock);
	}
}

void zvfs_free_fd(int fd)
{
	/* Assumes fd was already bounds-checked. */
	(void)z_fd_unref(fd);
}

int zvfs_alloc_fd(void *obj, const struct fd_op_vtable *vtable)
{
	int fd;

	fd = zvfs_reserve_fd();
	if (fd >= 0) {
		zvfs_finalize_fd(fd, obj, vtable);
	}

	return fd;
}

ssize_t zvfs_read(int fd, void *buf, size_t sz)
{
	ssize_t res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	(void)k_mutex_lock(&fdtable[fd].lock, K_FOREVER);
	res = fdtable[fd].vtable->read_offs(fdtable[fd].obj, buf, sz, fdtable[fd].offset);
	if (res > 0) {
		switch (fdtable[fd].mode & ZVFS_MODE_IFMT) {
		case ZVFS_MODE_IFDIR:
		case ZVFS_MODE_IFBLK:
		case ZVFS_MODE_IFSHM:
		case ZVFS_MODE_IFREG:
			fdtable[fd].offset += res;
			break;
		default:
			break;
		}
	}
	k_mutex_unlock(&fdtable[fd].lock);

	return res;
}

ssize_t zvfs_write(int fd, const void *buf, size_t sz)
{
	ssize_t res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	(void)k_mutex_lock(&fdtable[fd].lock, K_FOREVER);
	res = fdtable[fd].vtable->write_offs(fdtable[fd].obj, buf, sz, fdtable[fd].offset);
	if (res > 0) {
		switch (fdtable[fd].mode & ZVFS_MODE_IFMT) {
		case ZVFS_MODE_IFDIR:
		case ZVFS_MODE_IFBLK:
		case ZVFS_MODE_IFSHM:
		case ZVFS_MODE_IFREG:
			fdtable[fd].offset += res;
			break;
		default:
			break;
		}
	}
	k_mutex_unlock(&fdtable[fd].lock);

	return res;
}

int zvfs_close(int fd)
{
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	(void)k_mutex_lock(&fdtable[fd].lock, K_FOREVER);

	res = fdtable[fd].vtable->close(fdtable[fd].obj);

	k_mutex_unlock(&fdtable[fd].lock);

	zvfs_free_fd(fd);

	return res;
}

int zvfs_fstat(int fd, struct stat *buf)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return zvfs_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, ZFD_IOCTL_STAT, buf);
}

int zvfs_fsync(int fd)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return zvfs_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, ZFD_IOCTL_FSYNC);
}

static inline off_t zvfs_lseek_wrap(int fd, int cmd, ...)
{
	off_t res;
	va_list args;

	__ASSERT_NO_MSG(fd < ARRAY_SIZE(fdtable));

	(void)k_mutex_lock(&fdtable[fd].lock, K_FOREVER);
	va_start(args, cmd);
	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, cmd, args);
	va_end(args);
	if (res >= 0) {
		switch (fdtable[fd].mode & ZVFS_MODE_IFMT) {
		case ZVFS_MODE_IFDIR:
		case ZVFS_MODE_IFBLK:
		case ZVFS_MODE_IFSHM:
		case ZVFS_MODE_IFREG:
			fdtable[fd].offset = res;
			break;
		default:
			break;
		}
	}
	k_mutex_unlock(&fdtable[fd].lock);

	return res;
}

off_t zvfs_lseek(int fd, off_t offset, int whence)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return zvfs_lseek_wrap(fd, ZFD_IOCTL_LSEEK, offset, whence, fdtable[fd].offset);
}

int zvfs_fcntl(int fd, int cmd, va_list args)
{
	int res;

	if (_check_fd(fd) < 0) {
		return -1;
	}

	/* The rest of commands are per-fd, handled by ioctl vmethod. */
	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, cmd, args);

	return res;
}

static inline int zvfs_ftruncate_wrap(int fd, int cmd, ...)
{
	int res;
	va_list args;

	__ASSERT_NO_MSG(fd < ARRAY_SIZE(fdtable));

	(void)k_mutex_lock(&fdtable[fd].lock, K_FOREVER);
	va_start(args, cmd);
	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, cmd, args);
	va_end(args);
	k_mutex_unlock(&fdtable[fd].lock);

	return res;
}

int zvfs_ftruncate(int fd, off_t length)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return zvfs_ftruncate_wrap(fd, ZFD_IOCTL_TRUNCATE, length);
}

int zvfs_ioctl(int fd, unsigned long request, va_list args)
{
	if (_check_fd(fd) < 0) {
		return -1;
	}

	return fdtable[fd].vtable->ioctl(fdtable[fd].obj, request, args);
}


#if defined(CONFIG_POSIX_DEVICE_IO)
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
	return zvfs_write(1, buffer, count);
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

#endif /* defined(CONFIG_POSIX_DEVICE_IO) */
