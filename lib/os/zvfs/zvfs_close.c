/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/zvfs_fs.h>
#include <zephyr/internal/syscall_handler.h>

#ifdef CONFIG_ZVFS_UNLINK_ON_CLOSE
/* longest absolute path a marked description may carry */
#define ZVFS_UOC_PATH_LEN 16

/* descriptions whose path is removed on the final close, wherever it happens */
static struct {
	struct fd_entry *desc;
	char path[ZVFS_UOC_PATH_LEN];
} zvfs_uoc[CONFIG_ZVFS_UNLINK_ON_CLOSE_MAX];
static struct k_spinlock zvfs_uoc_lock;

int z_impl_zvfs_unlink_on_close(int fd, const char *path)
{
	int ret = -ENOSPC;
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		errno = EBADF;
		return -1;
	}
	if ((path == NULL) || (path[0] != '/')) {
		errno = EINVAL;
		return -1;
	}
	if (strlen(path) >= ZVFS_UOC_PATH_LEN) {
		errno = ENAMETOOLONG;
		return -1;
	}

	K_SPINLOCK(&zvfs_uoc_lock) {
		ARRAY_FOR_EACH(zvfs_uoc, i) {
			if (zvfs_uoc[i].desc == NULL) {
				zvfs_uoc[i].desc = entry;
				strcpy(zvfs_uoc[i].path, path);
				ret = 0;
				/* K_SPINLOCK_BREAK would resume this inner loop */
				break;
			}
		}
	}

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_unlink_on_close(int fd, const char *path)
{
	char kpath[ZVFS_UOC_PATH_LEN];

	if (zvfs_fd_syscall_verify(fd) == NULL) {
		errno = EBADF;
		return -1;
	}
	if (k_usermode_string_copy(kpath, path, sizeof(kpath)) != 0) {
		errno = ENAMETOOLONG;
		return -1;
	}

	return z_impl_zvfs_unlink_on_close(fd, kpath);
}

#include <zephyr/syscalls/zvfs_unlink_on_close_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* consume the mark for @a entry, if any, copying its path into @a path */
static void zvfs_uoc_consume(struct fd_entry *entry, char *path)
{
	path[0] = '\0';
	K_SPINLOCK(&zvfs_uoc_lock) {
		ARRAY_FOR_EACH(zvfs_uoc, i) {
			if (zvfs_uoc[i].desc == entry) {
				strcpy(path, zvfs_uoc[i].path);
				zvfs_uoc[i].desc = NULL;
				break;
			}
		}
	}
}
#endif /* CONFIG_ZVFS_UNLINK_ON_CLOSE */

int z_impl_zvfs_close(int fd)
{
	int res = 0;
#ifdef CONFIG_ZVFS_UNLINK_ON_CLOSE
	char upath[ZVFS_UOC_PATH_LEN] = "";
#endif /* CONFIG_ZVFS_UNLINK_ON_CLOSE */
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	(void)k_mutex_lock(&entry->lock, K_FOREVER);
	if (entry->vtable->close != NULL && entry->obj != NULL &&
	    zvfs_fd_obj_refcount(entry->obj) <= 1) {
		if (entry->mode & ZVFS_MODE_IFSOCK) {
			res = entry->vtable->close2(entry->obj, fd);
			zvfs_socket_close_hook(fd);
		} else {
			res = entry->vtable->close(entry->obj);
		}
#ifdef CONFIG_ZVFS_UNLINK_ON_CLOSE
		/* consumed before the entry can be recycled; removed after unlock */
		zvfs_uoc_consume(entry, upath);
#endif /* CONFIG_ZVFS_UNLINK_ON_CLOSE */
	}
	k_mutex_unlock(&entry->lock);

#ifdef CONFIG_ZVFS_UNLINK_ON_CLOSE
	if (upath[0] != '\0') {
		(void)zvfs_unlink(upath);
	}
#endif /* CONFIG_ZVFS_UNLINK_ON_CLOSE */

	zvfs_free_fd(fd);

	return res;
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_close(int fd)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	return z_impl_zvfs_close(fd);
}

#include <zephyr/syscalls/zvfs_close_mrsh.c>
#endif /* CONFIG_USERSPACE */
