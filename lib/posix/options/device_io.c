/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/posix/poll.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/select.h>

/* prototypes for external, not-yet-public, functions in fdtable.c or fs.c */
int zvfs_close(int fd);
int zvfs_open(const char *name, int flags);
ssize_t zvfs_read(int fd, void *buf, size_t sz, size_t *from_offset);
ssize_t zvfs_write(int fd, const void *buf, size_t sz, size_t *from_offset);

void FD_CLR(int fd, struct zvfs_fd_set *fdset)
{
	return ZVFS_FD_CLR(fd, (struct zvfs_fd_set *)fdset);
}

int FD_ISSET(int fd, struct zvfs_fd_set *fdset)
{
	return ZVFS_FD_ISSET(fd, (struct zvfs_fd_set *)fdset);
}

void FD_SET(int fd, struct zvfs_fd_set *fdset)
{
	ZVFS_FD_SET(fd, (struct zvfs_fd_set *)fdset);
}

void FD_ZERO(fd_set *fdset)
{
	ZVFS_FD_ZERO((struct zvfs_fd_set *)fdset);
}

int close(int fd)
{
	return zvfs_close(fd);
}
#ifdef CONFIG_POSIX_DEVICE_IO_ALIAS_CLOSE
FUNC_ALIAS(close, _close, int);
#endif

int open(const char *name, int flags, ...)
{
	/* FIXME: necessarily need to check for O_CREAT and unpack ... if set */
	return zvfs_open(name, flags);
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
