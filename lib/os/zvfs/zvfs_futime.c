/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <time.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/zvfs_fs.h>
#include <zephyr/internal/syscall_handler.h>

static inline int zvfs_futime_wrap(struct fd_entry *entry, int cmd, ...)
{
	int res;
	va_list args;

	(void)k_mutex_lock(&entry->lock, K_FOREVER);
	va_start(args, cmd);
	res = entry->vtable->ioctl(entry->obj, cmd, args);
	va_end(args);
	k_mutex_unlock(&entry->lock);

	return res;
}

int z_impl_zvfs_futime(int fd, const struct timespec *times)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	return zvfs_futime_wrap(entry, ZFD_IOCTL_UTIME, times);
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_futime(int fd, const struct timespec *times)
{
	struct timespec ktimes[2];
	const struct timespec *tp = NULL;

	if (times != NULL) {
		K_OOPS(k_usermode_from_copy(ktimes, times, sizeof(ktimes)));
		tp = ktimes;
	}

	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	return z_impl_zvfs_futime(fd, tp);
}

#include <zephyr/syscalls/zvfs_futime_mrsh.c>
#endif /* CONFIG_USERSPACE */
