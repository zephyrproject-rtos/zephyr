/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>

int zvfs_fcntl(int fd, int cmd, va_list args)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	return entry->vtable->ioctl(entry->obj, cmd, args);
}
