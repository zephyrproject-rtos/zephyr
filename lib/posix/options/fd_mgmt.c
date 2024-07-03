/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/select.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/sys/fdtable.h>

/* prototypes for external, not-yet-public, functions in fdtable.c or fs.c */
int zvfs_dup(int fd, int *newfd);
int zvfs_fcntl(int fd, int cmd, va_list arg);
int zvfs_fileno(FILE *file);
int zvfs_ftruncate(int fd, off_t length);
off_t zvfs_lseek(int fd, off_t offset, int whence);

int dup(int fd)
{
	return zvfs_dup(fd, NULL);
}

int dup2(int oldfd, int newfd)
{
	return zvfs_dup(oldfd, &newfd);
}

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

int fseeko(FILE *file, off_t offset, int whence)
{
	int fd;

	fd = zvfs_fileno(file);
	if (fd < 0) {
		return -1;
	}

	return zvfs_lseek(fd, offset, whence);
}

off_t ftello(FILE *file)
{
	int fd;

	fd = zvfs_fileno(file);
	if (fd < 0) {
		return -1;
	}

	return zvfs_lseek(fd, 0, SEEK_CUR);
}

int ftruncate(int fd, off_t length)
{
	return zvfs_ftruncate(fd, length);
}
#ifdef CONFIG_POSIX_FD_MGMT_ALIAS_FTRUNCATE
FUNC_ALIAS(ftruncate, _ftruncate, int);
#endif /* CONFIG_POSIX_FD_MGMT_ALIAS_FTRUNCATE */

off_t lseek(int fd, off_t offset, int whence)
{
	return zvfs_lseek(fd, offset, whence);
}
#ifdef CONFIG_POSIX_FD_MGMT_ALIAS_LSEEK
FUNC_ALIAS(lseek, _lseek, off_t);
#endif /* CONFIG_POSIX_FD_MGMT_ALIAS_LSEEK */
