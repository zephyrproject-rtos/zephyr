/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/internal/syscall_handler.h>

int z_impl_zvfs_fstat(int fd, struct zvfs_stat *buf)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	return zvfs_fdtable_call_ioctl(entry->vtable, entry->obj, ZFD_IOCTL_STAT, buf);
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_fstat(int fd, struct zvfs_stat *buf)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, sizeof(struct zvfs_stat)));

	return z_impl_zvfs_fstat(fd, buf);
}

#include <zephyr/syscalls/zvfs_fstat_mrsh.c>
#endif /* CONFIG_USERSPACE */
