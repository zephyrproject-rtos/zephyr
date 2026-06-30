/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2024 Tenstorrent AI ULC
 * Copyright (c) 2025 Antmicro
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
#include <stdio.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/kobject_internal.h>
#include <zephyr/sys/speculation.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/fs/fs.h>

K_MEM_SLAB_DEFINE_TYPE(file_desc_slab, struct fs_file_t, ZVFS_OPEN_SIZE);

#ifdef CONFIG_ZVFS_RESERVE_STDIO_FDS
static const struct fd_op_vtable stdinout_fd_op_vtable;
BUILD_ASSERT(ZVFS_OPEN_SIZE >= 3, "ZVFS_OPEN_SIZE >= 3 to reserve stdin/stdout/stderr fds");
#endif

static struct fd_entry fdtable[ZVFS_OPEN_SIZE] = {
#ifdef CONFIG_ZVFS_RESERVE_STDIO_FDS
	/*
	 * Predefine entries for stdin/stdout/stderr.
	 */
	{
		/* STDIN */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[0].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[0].cond),
		.mode = ZVFS_MODE_IFCHR,
	},
	{
		/* STDOUT */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[1].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[1].cond),
		.mode = ZVFS_MODE_IFCHR,
	},
	{
		/* STDERR */
		.vtable = &stdinout_fd_op_vtable,
		.refcount = ATOMIC_INIT(1),
		.lock = Z_MUTEX_INITIALIZER(fdtable[2].lock),
		.cond = Z_CONDVAR_INITIALIZER(fdtable[2].cond),
		.mode = ZVFS_MODE_IFCHR,
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

#ifdef CONFIG_USERSPACE
	{
		struct k_object *ko = k_object_find(&fdtable[fd]);

		/* closed => no thread may retain a grant on this slot */
		if (ko != NULL) {
			(void)memset(ko->perms, 0, sizeof(ko->perms));
			ko->flags &= ~(K_OBJ_FLAG_INITIALIZED | K_OBJ_FLAG_PUBLIC);
		}
	}
#endif

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

static int _find_fd_entry_at_or_above(int minfd)
{
	int fd;

	if (minfd < 0) {
		errno = EINVAL;
		return -1;
	}

	for (fd = minfd; fd < ARRAY_SIZE(fdtable); fd++) {
		if (!atomic_get(&fdtable[fd].refcount)) {
			return fd;
		}
	}

	errno = EMFILE;
	return -1;
}

int zvfs_fd_obj_refcount(void *obj)
{
	int count = 0;

	if (obj == NULL) {
		return 0;
	}

	for (int fd = 0; fd < ARRAY_SIZE(fdtable); fd++) {
		if (atomic_get(&fdtable[fd].refcount) && fdtable[fd].obj == obj) {
			count++;
		}
	}

	return count;
}

void zvfs_fd_offset_set(struct fd_entry *entry, size_t offset)
{
	void *obj = entry->obj;

	if (obj == NULL) {
		entry->offset = offset;
		return;
	}

	for (int fd = 0; fd < ARRAY_SIZE(fdtable); fd++) {
		if (atomic_get(&fdtable[fd].refcount) && fdtable[fd].obj == obj) {
			fdtable[fd].offset = offset;
		}
	}
}

static int zvfs_dup_entry_locked(struct fd_entry *src, int newfd)
{
#if defined(CONFIG_USERSPACE)
	k_object_recycle(&fdtable[newfd]);
#endif
	(void)z_fd_ref(newfd);
	fdtable[newfd].obj = src->obj;
	fdtable[newfd].vtable = src->vtable;
	fdtable[newfd].mode = src->mode;
	fdtable[newfd].offset = src->offset;
	k_mutex_init(&fdtable[newfd].lock);
	k_condvar_init(&fdtable[newfd].cond);

	return newfd;
}

int z_impl_zvfs_dup(int oldfd, int minfd)
{
	struct fd_entry *src = zvfs_fd_entry_get(oldfd);
	int fd;

	if (src == NULL) {
		return -1;
	}

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	fd = _find_fd_entry_at_or_above(minfd);
	if (fd >= 0) {
		fd = zvfs_dup_entry_locked(src, fd);
	}

	k_mutex_unlock(&fdtable_lock);

	return fd;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_dup(int oldfd, int minfd)
{
	if (zvfs_fd_syscall_verify(oldfd) == NULL) {
		return -1;
	}

	return z_impl_zvfs_dup(oldfd, minfd);
}
#include <zephyr/syscalls/zvfs_dup_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_dup2(int oldfd, int newfd)
{
	struct fd_entry *src;

	if (oldfd == newfd) {
		if (zvfs_fd_entry_get(oldfd) == NULL) {
			return -1;
		}

		return newfd;
	}

	src = zvfs_fd_entry_get(oldfd);
	if (src == NULL) {
		return -1;
	}

	if (newfd < 0 || newfd >= ARRAY_SIZE(fdtable)) {
		errno = EBADF;
		return -1;
	}

	if (zvfs_fd_entry_get(newfd) != NULL) {
		if (zvfs_close(newfd) < 0) {
			return -1;
		}
	}

	(void)k_mutex_lock(&fdtable_lock, K_FOREVER);

	if (atomic_get(&fdtable[newfd].refcount)) {
		errno = EBADF;
		k_mutex_unlock(&fdtable_lock);
		return -1;
	}

	newfd = zvfs_dup_entry_locked(src, newfd);

	k_mutex_unlock(&fdtable_lock);

	return newfd;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_dup2(int oldfd, int newfd)
{
	if (zvfs_fd_syscall_verify(oldfd) == NULL) {
		return -1;
	}

	/*
	 * The implementation closes an in-use newfd via z_impl_zvfs_close(),
	 * which skips the ownership check; verify it here so a user thread
	 * cannot close a descriptor it has no K_OBJ_FILE grant for. An unused
	 * newfd returns NULL, which is not an error.
	 */
	if (oldfd != newfd) {
		(void)zvfs_fd_syscall_verify(newfd);
	}

	return z_impl_zvfs_dup2(oldfd, newfd);
}
#include <zephyr/syscalls/zvfs_dup2_mrsh.c>
#endif /* CONFIG_USERSPACE */

struct fd_entry *zvfs_fd_entry_get(int fd)
{
	if ((fd < 0) || (fd >= ARRAY_SIZE(fdtable))) {
		errno = EBADF;
		return NULL;
	}

	fd = k_array_index_sanitize(fd, ARRAY_SIZE(fdtable));

	if (!atomic_get(&fdtable[fd].refcount)) {
		errno = EBADF;
		return NULL;
	}

	return &fdtable[fd];
}

struct fd_entry *zvfs_fd_syscall_verify(int fd)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

#if defined(CONFIG_USERSPACE)
	if (entry != NULL && k_is_in_user_syscall()) {
		K_OOPS(K_SYSCALL_OBJ(entry, K_OBJ_FILE));
	}
#endif

	return entry;
}

static int _check_fd(int fd)
{
	if (zvfs_fd_entry_get(fd) == NULL) {
		return -1;
	}

	return 0;
}

#if defined(CONFIG_ZTEST) && defined(CONFIG_USERSPACE)
void *zvfs_test_fd_entry_kobj(int fd)
{
	if ((fd < 0) || (fd >= ARRAY_SIZE(fdtable))) {
		return NULL;
	}

	return &fdtable[k_array_index_sanitize(fd, ARRAY_SIZE(fdtable))];
}
#endif

void *zvfs_get_fd_obj(int fd, const struct fd_op_vtable *vtable, int err)
{
	struct fd_entry *entry;

	entry = zvfs_fd_syscall_verify(fd);
	if (entry == NULL) {
		return NULL;
	}

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

void *zvfs_get_fd_obj_and_vtable(int fd, const struct fd_op_vtable **vtable, struct k_mutex **lock)
{
	struct fd_entry *entry;

	entry = zvfs_fd_syscall_verify(fd);
	if (entry == NULL) {
		return NULL;
	}

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
#if defined(CONFIG_USERSPACE)
		k_object_recycle(&fdtable[fd]);
#endif
		/* Mark entry as used, zvfs_finalize_fd() will fill it in. */
		(void)z_fd_ref(fd);
		fdtable[fd].obj = NULL;
		fdtable[fd].vtable = NULL;
		fdtable[fd].offset = 0;
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
		int prev_errno = errno;

		(void)zvfs_fdtable_call_ioctl(vtable, obj, ZFD_IOCTL_SET_LOCK, &fdtable[fd].lock);
		if ((prev_errno != EOPNOTSUPP) && (errno == EOPNOTSUPP)) {
			/* restore backed-up errno value if the backend does not support locking */
			errno = prev_errno;
		}
	}
}

void zvfs_free_fd(int fd)
{
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

FILE *zvfs_fdopen(int fd, const char *mode)
{
	ARG_UNUSED(mode);

	if (_check_fd(fd) < 0) {
		return NULL;
	}

	return (FILE *)&fdtable[fd];
}

int zvfs_fileno(FILE *file)
{
	if (!IS_ARRAY_ELEMENT(fdtable, file)) {
		errno = EBADF;
		return -1;
	}

	return (struct fd_entry *)file - fdtable;
}

int zvfs_unlink(const char *path)
{
	int res = fs_unlink(path);

	if (res < 0) {
		errno = -res;
		return -1;
	}
	return 0;
}

int zvfs_rename(const char *old, const char *newp)
{
	int res = fs_rename(old, newp);

	if (res < 0) {
		errno = -res;
		return -1;
	}
	return 0;
}

#if defined(CONFIG_POSIX_DEVICE_IO) || defined(CONFIG_ZVFS_RESERVE_STDIO_FDS)
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
#if defined(CONFIG_PICOLIBC) || defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_ARCMWDT_LIBC) || defined(CONFIG_MINIMAL_LIBC)
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

#endif /* defined(CONFIG_POSIX_DEVICE_IO) || defined(CONFIG_ZVFS_RESERVE_STDIO_FDS) */

#if defined(CONFIG_USERSPACE) && defined(CONFIG_ZVFS_RESERVE_STDIO_FDS)
static int stdinout_access_init(void)
{
	/* all threads share stdin/stdout/stderr */
	for (int fd = 0; fd < 3; fd++) {
		k_object_init(&fdtable[fd]);
		k_object_access_all_grant(&fdtable[fd]);
	}

	return 0;
}
SYS_INIT(stdinout_access_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
