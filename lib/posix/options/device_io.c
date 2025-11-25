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
FILE *zvfs_fdopen(int fd, const char *mode);
int zvfs_fileno(FILE *file);

static struct fd_op_vtable posix_op_vtable = {
	.read = zvfs_read_vmeth,
	.write = zvfs_write_vmeth,
	.close = zvfs_close_vmeth,
	.ioctl = zvfs_ioctl_vmeth,
};

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

static int posix_mode_to_zephyr(int mf)
{
	int mode = (mf & O_CREAT) ? FS_O_CREATE : 0;

	mode |= (mf & O_APPEND) ? FS_O_APPEND : 0;
	mode |= (mf & O_TRUNC) ? FS_O_TRUNC : 0;

	switch (mf & O_ACCMODE) {
	case O_RDONLY:
		mode |= FS_O_READ;
		break;
	case O_WRONLY:
		mode |= FS_O_WRITE;
		break;
	case O_RDWR:
		mode |= FS_O_RDWR;
		break;
	default:
		break;
	}

	return mode;
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
	int zflags = posix_mode_to_zephyr(flags);

	return zvfs_open(name, zflags, &posix_op_vtable);
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
