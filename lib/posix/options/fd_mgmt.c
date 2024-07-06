/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/select.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/sys/fdtable.h>

/* prototypes for external, not-yet-public, functions in fdtable.c or fs.c */
int zvfs_fcntl(int fd, int cmd, va_list arg);
int zvfs_ftruncate(int fd, k_off_t length);
k_off_t zvfs_lseek(int fd, k_off_t offset, int whence);

int fcntl(int fd, int cmd, ...)
{
	int ret;
	va_list args;

	va_start(args, cmd);
	ret = zvfs_fcntl(fd, cmd, args);
	va_end(args);

	return ret;
}
#ifdef CONFIG_POSIX_FD_MGMT_ALIAS_FCNTL
FUNC_ALIAS(fcntl, _fcntl, int);
#endif /* CONFIG_POSIX_FD_MGMT_ALIAS_FCNTL */

int ftruncate(int fd, off_t length)
{
	return zvfs_ftruncate(fd, (k_off_t)length);
}
#ifdef CONFIG_POSIX_FD_MGMT_ALIAS_FTRUNCATE
FUNC_ALIAS(ftruncate, _ftruncate, int);
#endif /* CONFIG_POSIX_FD_MGMT_ALIAS_FTRUNCATE */

off_t lseek(int fd, off_t offset, int whence)
{
	return zvfs_lseek(fd, (k_off_t)offset, whence);
}
#ifdef CONFIG_POSIX_FD_MGMT_ALIAS_LSEEK
FUNC_ALIAS(lseek, _lseek, off_t);
#endif /* CONFIG_POSIX_FD_MGMT_ALIAS_LSEEK */
