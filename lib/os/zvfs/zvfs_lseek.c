/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/internal/syscall_handler.h>

static inline off_t zvfs_lseek_wrap(struct fd_entry *entry, int cmd, ...)
{
	off_t res;
	va_list args;

	(void)k_mutex_lock(&entry->lock, K_FOREVER);
	va_start(args, cmd);
	res = entry->vtable->ioctl(entry->obj, cmd, args);
	va_end(args);
	if (res >= 0) {
		switch (entry->mode & ZVFS_MODE_IFMT) {
		case ZVFS_MODE_IFDIR:
		case ZVFS_MODE_IFBLK:
		case ZVFS_MODE_IFSHM:
		case ZVFS_MODE_IFREG:
			entry->offset = res;
			break;
		default:
			break;
		}
	}
	k_mutex_unlock(&entry->lock);

	return res;
}

off_t z_impl_zvfs_lseek(int fd, off_t offset, int whence)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	return zvfs_lseek_wrap(entry, ZFD_IOCTL_LSEEK, offset, whence, entry->offset);
}

#ifdef CONFIG_USERSPACE
static off_t z_vrfy_zvfs_lseek(int fd, off_t offset, int whence)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	return z_impl_zvfs_lseek(fd, offset, whence);
}

#include <zephyr/syscalls/zvfs_lseek_mrsh.c>
#endif /* CONFIG_USERSPACE */
