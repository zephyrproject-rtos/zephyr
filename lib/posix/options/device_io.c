/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/select.h>

/* prototypes for external, not-yet-public, functions in fdtable.c or fs.c */
int zvfs_close(int fd);
FILE *zvfs_fdopen(int fd, const char *mode);
int zvfs_fileno(FILE *file);
int zvfs_open(const char *name, int flags, int mode);
ssize_t zvfs_read(int fd, void *buf, size_t sz, size_t *from_offset);
ssize_t zvfs_write(int fd, const void *buf, size_t sz, size_t *from_offset);

void FD_CLR(int fd, struct zvfs_fd_set *fdset)
{
	return ZVFS_FD_CLR(fd, fdset);
}

int FD_ISSET(int fd, struct zvfs_fd_set *fdset)
{
	return ZVFS_FD_ISSET(fd, fdset);
}

void FD_SET(int fd, struct zvfs_fd_set *fdset)
{
	ZVFS_FD_SET(fd, fdset);
}

void FD_ZERO(fd_set *fdset)
{
	ZVFS_FD_ZERO(fdset);
}

int close(int fd)
{
	return zvfs_close(fd);
}
#ifdef CONFIG_POSIX_DEVICE_IO_ALIAS_CLOSE
FUNC_ALIAS(close, _close, int);
#endif

FILE *fdopen(int fd, const char *mode)
{
	return zvfs_fdopen(fd, mode);
}

int fileno(FILE *file)
{
	return zvfs_fileno(file);
}

int open(const char *name, int flags, ...)
{
	int mode = 0;
	va_list args;

	if ((flags & O_CREAT) != 0) {
		va_start(args, flags);
		mode = va_arg(args, int);
		va_end(args);
	}

	return zvfs_open(name, flags, mode);
}
#ifdef CONFIG_POSIX_DEVICE_IO_ALIAS_OPEN
FUNC_ALIAS(open, _open, int);
#endif

int poll(struct pollfd *fds, int nfds, int timeout)
{
	return zvfs_poll(fds, nfds, timeout);
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
	size_t off = (size_t)offset;

	if (offset < 0) {
		errno = EINVAL;
		return -1;
	}

	return zvfs_read(fd, buf, count, (size_t *)&off);
}

int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    const struct timespec *timeout, const void *sigmask)
{
	return zvfs_select(nfds, readfds, writefds, exceptfds, timeout, sigmask);
}

ssize_t pwrite(int fd, void *buf, size_t count, off_t offset)
{
	size_t off = (size_t)offset;

	if (offset < 0) {
		errno = EINVAL;
		return -1;
	}

	return zvfs_write(fd, buf, count, (size_t *)&off);
}

ssize_t read(int fd, void *buf, size_t sz)
{
	return zvfs_read(fd, buf, sz, NULL);
}
#ifdef CONFIG_POSIX_DEVICE_IO_ALIAS_READ
FUNC_ALIAS(read, _read, ssize_t);
#endif

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	struct timespec to = {
		.tv_sec = (timeout == NULL) ? 0 : timeout->tv_sec,
		.tv_nsec = (long)((timeout == NULL) ? 0 : timeout->tv_usec * NSEC_PER_USEC)};

	return zvfs_select(nfds, readfds, writefds, exceptfds, (timeout == NULL) ? NULL : &to,
			   NULL);
}

ssize_t write(int fd, const void *buf, size_t sz)
{
	return zvfs_write(fd, buf, sz, NULL);
}
#ifdef CONFIG_POSIX_DEVICE_IO_ALIAS_WRITE
FUNC_ALIAS(write, _write, ssize_t);
#endif
