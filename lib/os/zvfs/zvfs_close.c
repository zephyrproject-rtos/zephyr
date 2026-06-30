/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/internal/syscall_handler.h>

int z_impl_zvfs_close(int fd)
{
	int res = 0;
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
	}
	k_mutex_unlock(&entry->lock);

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
