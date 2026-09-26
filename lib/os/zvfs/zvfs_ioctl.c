/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/internal/syscall_handler.h>

int z_impl_zvfs_ioctl(int fd, unsigned long request, va_list args)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	return entry->vtable->ioctl(entry->obj, request, args);
}

#ifdef CONFIG_USERSPACE
/*
 * Requests not admitted here are reachable from user mode only through their
 * dedicated, pointer-validated system calls (zvfs_fstat(), zvfs_lseek(), ...).
 */
static int z_vrfy_zvfs_ioctl(int fd, unsigned long request, va_list args)
{
	va_list check;
	int ret = 0;

	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	va_copy(check, args);

	switch (request) {
	case ZFD_IOCTL_FIONBIO:
		break;
	case ZFD_IOCTL_FIONREAD: {
		int *avail = va_arg(check, int *);

		K_OOPS(K_SYSCALL_MEMORY_WRITE(avail, sizeof(*avail)));
	} break;
	default:
		errno = EOPNOTSUPP;
		ret = -1;
		break;
	}

	va_end(check);

	if (ret < 0) {
		return ret;
	}

	return z_impl_zvfs_ioctl(fd, request, args);
}

#include <zephyr/syscalls/zvfs_ioctl_mrsh.c>
#endif /* CONFIG_USERSPACE */
