/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/internal/syscall_handler.h>

int z_impl_zvfs_fsync(int fd)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	return zvfs_fdtable_call_ioctl(entry->vtable, entry->obj, ZFD_IOCTL_FSYNC);
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_fsync(int fd)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	return z_impl_zvfs_fsync(fd);
}

#include <zephyr/syscalls/zvfs_fsync_mrsh.c>
#endif /* CONFIG_USERSPACE */
