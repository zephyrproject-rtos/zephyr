/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/internal/syscall_handler.h>

int z_impl_zvfs_fcntl(int fd, int cmd, uintptr_t arg)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	switch (cmd) {
	case ZVFS_F_GETFD:
		return entry->fd_flags;

	case ZVFS_F_SETFD:
		entry->fd_flags = (int)arg & ZVFS_FD_CLOEXEC;
		return 0;

	case ZVFS_F_DUPFD:
		return zvfs_dup(fd, (int)arg);

	case ZVFS_F_DUPFD_CLOEXEC: {
		struct fd_entry *dup_entry;
		int dupfd;

		dupfd = zvfs_dup(fd, (int)arg);
		if (dupfd < 0) {
			return -1;
		}

		dup_entry = zvfs_fd_entry_get(dupfd);
		if (dup_entry == NULL) {
			(void)zvfs_close(dupfd);
			return -1;
		}

		dup_entry->fd_flags = ZVFS_FD_CLOEXEC;
		return dupfd;
	}

	case ZVFS_F_GETLK:
	case ZVFS_F_SETLK:
	case ZVFS_F_SETLKW: {
		struct zvfs_flock *lock = (struct zvfs_flock *)(uintptr_t)arg;

		if (lock == NULL) {
			errno = EINVAL;
			return -1;
		}

#ifdef CONFIG_ZVFS_FS
		return zvfs_record_lock_fcntl(entry, cmd, lock);
#else
		errno = EOPNOTSUPP;
		return -1;
#endif
	}

	default:
		return zvfs_fdtable_call_ioctl(entry->vtable, entry->obj, cmd, (int)arg);
	}
}

#ifdef CONFIG_USERSPACE
static int z_vrfy_zvfs_fcntl(int fd, int cmd, uintptr_t arg)
{
	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	switch (cmd) {
	case ZVFS_F_GETLK:
		K_OOPS(K_SYSCALL_MEMORY_WRITE((void *)arg, sizeof(struct zvfs_flock)));
		break;
	case ZVFS_F_SETLK:
	case ZVFS_F_SETLKW:
		K_OOPS(K_SYSCALL_MEMORY_READ((void *)arg, sizeof(struct zvfs_flock)));
		break;
	default:
		break;
	}

	return z_impl_zvfs_fcntl(fd, cmd, arg);
}

#include <zephyr/syscalls/zvfs_fcntl_mrsh.c>
#endif /* CONFIG_USERSPACE */
