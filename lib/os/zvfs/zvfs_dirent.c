/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ZVFS directory streams backed by file descriptors.
 *
 * Directory streams are opened as K_OBJ_FILE file descriptors so that user
 * threads can enumerate directories through verified system calls. The
 * stream objects themselves live in zvfs_dir_partition so a user thread can
 * read the entry storage directly; allocation bookkeeping stays kernel-side,
 * inside the zvfs_opendir() and close paths.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/fs/fs.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/zvfs_fs.h>

#include "zvfs_fs_priv.h"

K_APPMEM_PARTITION_DEFINE(zvfs_dir_partition);

static K_APP_DMEM(zvfs_dir_partition) struct zvfs_dirstream
	zvfs_dir_storage[CONFIG_ZVFS_DIR_MAX];

SYS_ELASTIPOOL_DEFINE_ADVANCED(zvfs_dir_pool, sizeof(struct zvfs_dirstream),
			       __alignof(struct zvfs_dirstream), CONFIG_ZVFS_DIR_MAX,
			       CONFIG_ZVFS_DIR_MAX, NULL, zvfs_dir_storage, static);

void zvfs_dirstream_free(struct zvfs_dirstream *d)
{
	(void)sys_elastipool_free(&zvfs_dir_pool, d);
}

struct zvfs_dirstream *z_impl_zvfs_opendir(const char *path)
{
	int rc, fd;
	void *obj = NULL;
	struct zvfs_dirstream *d = NULL;
	struct zvfs_fs_desc *ptr = NULL;

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		return NULL;
	}

	rc = sys_elastipool_alloc(&zvfs_dir_pool, &obj);
	if (rc < 0) {
		rc = -ENOMEM;
		goto out_err;
	}
	d = obj;

	ptr = zvfs_fs_desc_alloc(true);
	if (ptr == NULL) {
		rc = -EMFILE;
		goto out_err;
	}

	fs_dir_t_init(&ptr->dir);

	rc = fs_opendir(&ptr->dir, path);
	if (rc < 0) {
		goto out_err;
	}

	memset(d, 0, sizeof(*d));
	d->fd = fd;
	ptr->stream = d;

	zvfs_finalize_typed_fd(fd, ptr, &fs_dir_fd_op_vtable, ZVFS_MODE_IFDIR);

	return d;

out_err:
	if (ptr != NULL) {
		zvfs_fs_desc_free(ptr);
	}
	if (d != NULL) {
		zvfs_dirstream_free(d);
	}
	zvfs_free_fd(fd);
	errno = -rc;
	return NULL;
}

#ifdef CONFIG_USERSPACE
static struct zvfs_dirstream *z_vrfy_zvfs_opendir(const char *path)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return NULL;
	}

	return z_impl_zvfs_opendir(kpath);
}
#include <zephyr/syscalls/zvfs_opendir_mrsh.c>
#endif /* CONFIG_USERSPACE */

static int zvfs_dir_fd_ioctl(int fd, unsigned long request, void *arg)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	if ((entry->mode & ZVFS_MODE_IFMT) != ZVFS_MODE_IFDIR) {
		errno = ENOTDIR;
		return -1;
	}

	return zvfs_fdtable_call_ioctl(entry->vtable, entry->obj, request, arg);
}

int z_impl_zvfs_readdir(int fd, struct zvfs_dirent *ent)
{
	return zvfs_dir_fd_ioctl(fd, ZFD_IOCTL_READDIR, ent);
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_readdir(int fd, struct zvfs_dirent *ent)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	K_OOPS(K_SYSCALL_MEMORY_WRITE(ent, sizeof(*ent)));

	return z_impl_zvfs_readdir(fd, ent);
}
#include <zephyr/syscalls/zvfs_readdir_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_rewinddir(int fd)
{
	return zvfs_dir_fd_ioctl(fd, ZFD_IOCTL_REWINDDIR, NULL);
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_rewinddir(int fd)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	return z_impl_zvfs_rewinddir(fd);
}
#include <zephyr/syscalls/zvfs_rewinddir_mrsh.c>
#endif /* CONFIG_USERSPACE */
