/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ZVFS current working directory.
 *
 * Zephyr is a single process, so the working directory is a single
 * system-wide string, as POSIX defines it per-process. The lock is held only
 * to snapshot or store the string, never across a file system operation.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/fs/fs.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/zvfs_fs.h>

#include "zvfs_fs_priv.h"

static char zvfs_cwd[CONFIG_ZVFS_PATH_MAX] = "/";
static K_MUTEX_DEFINE(zvfs_cwd_lock);

/*
 * Resolution is purely lexical: '.', '..', and repeated '/' are collapsed
 * without consulting the file system. On a backend that supports symbolic
 * links (such as ext2) this is a simplification rather than full POSIX
 * pathname resolution. The output always begins with '/', has no '.', '..',
 * or repeated '/' components, and has no trailing '/' except for the root
 * itself.
 */
int zvfs_resolve_path(const char *in, char *out, size_t outsz)
{
	size_t o = 0;
	const char *p = in;

	if (in == NULL) {
		return -EFAULT;
	}

	if (in[0] == '\0') {
		return -ENOENT;
	}

	if (in[0] != '/') {
		k_mutex_lock(&zvfs_cwd_lock, K_FOREVER);
		o = strlen(zvfs_cwd);
		if (o >= outsz) {
			k_mutex_unlock(&zvfs_cwd_lock);
			return -ENAMETOOLONG;
		}
		memcpy(out, zvfs_cwd, o + 1);
		k_mutex_unlock(&zvfs_cwd_lock);

		if (o == 1) {
			/* the root: seed with the empty prefix */
			o = 0;
		}
	}

	while (*p != '\0') {
		const char *start;
		size_t len;

		while (*p == '/') {
			p++;
		}
		if (*p == '\0') {
			break;
		}

		start = p;
		while (*p != '\0' && *p != '/') {
			p++;
		}
		len = p - start;

		if (len == 1 && start[0] == '.') {
			continue;
		}

		if (len == 2 && start[0] == '.' && start[1] == '.') {
			/* pop the previous component; ".." at the root is a no-op */
			while (o > 0 && out[o - 1] != '/') {
				o--;
			}
			if (o > 0) {
				o--;
			}
			continue;
		}

		if (o + 1 + len + 1 > outsz) {
			return -ENAMETOOLONG;
		}

		out[o++] = '/';
		memcpy(&out[o], start, len);
		o += len;
	}

	if (o == 0) {
		out[o++] = '/';
	}
	out[o] = '\0';

	return 0;
}

int z_impl_zvfs_chdir(const char *path)
{
	int rc;
	struct fs_dirent entry;
	char r[CONFIG_ZVFS_PATH_MAX];

	rc = zvfs_resolve_path(path, r, sizeof(r));
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	rc = fs_stat(r, &entry);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	if (entry.type != FS_DIR_ENTRY_DIR) {
		errno = ENOTDIR;
		return -1;
	}

	k_mutex_lock(&zvfs_cwd_lock, K_FOREVER);
	strcpy(zvfs_cwd, r);
	k_mutex_unlock(&zvfs_cwd_lock);

	return 0;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_chdir(const char *path)
{
	char kpath[CONFIG_ZVFS_PATH_MAX];

	if (zvfs_copy_user_path(kpath, path) < 0) {
		return -1;
	}

	return z_impl_zvfs_chdir(kpath);
}
#include <zephyr/syscalls/zvfs_chdir_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_fchdir(int fd)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);
	struct zvfs_fs_desc *desc;

	if (entry == NULL) {
		return -1;
	}

	if ((entry->mode & ZVFS_MODE_IFMT) != ZVFS_MODE_IFDIR) {
		errno = ENOTDIR;
		return -1;
	}

	desc = entry->obj;
	if (desc == NULL || desc->path[0] == '\0') {
		errno = EBADF;
		return -1;
	}

	k_mutex_lock(&zvfs_cwd_lock, K_FOREVER);
	strcpy(zvfs_cwd, desc->path);
	k_mutex_unlock(&zvfs_cwd_lock);

	return 0;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_fchdir(int fd)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	return z_impl_zvfs_fchdir(fd);
}
#include <zephyr/syscalls/zvfs_fchdir_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_zvfs_getcwd(char *buf, size_t size)
{
	size_t len;

	if (buf == NULL || size == 0) {
		errno = EINVAL;
		return -1;
	}

	k_mutex_lock(&zvfs_cwd_lock, K_FOREVER);
	len = strlen(zvfs_cwd);
	if (len >= size) {
		k_mutex_unlock(&zvfs_cwd_lock);
		errno = ERANGE;
		return -1;
	}
	memcpy(buf, zvfs_cwd, len + 1);
	k_mutex_unlock(&zvfs_cwd_lock);

	return 0;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_getcwd(char *buf, size_t size)
{
	if (buf != NULL && size != 0) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, size));
	}

	return z_impl_zvfs_getcwd(buf, size);
}
#include <zephyr/syscalls/zvfs_getcwd_mrsh.c>
#endif /* CONFIG_USERSPACE */
