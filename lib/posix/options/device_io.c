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
#include <zephyr/posix/sys/socket.h>

/* prototypes for external, not-yet-public, functions in fdtable.c or fs.c */
int zvfs_close(int fd);
int zvfs_open(const char *name, int flags);
ssize_t zvfs_read(int fd, void *buf, size_t sz);
ssize_t zvfs_write(int fd, const void *buf, size_t sz);

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
	/* TODO: create  zvfs_poll() and dispatch to subsystems based on file type */
	return zsock_poll(fds, nfds, timeout);
}

ssize_t read(int fd, void *buf, size_t sz)
{
	return zvfs_read(fd, buf, sz);
}
#ifdef CONFIG_POSIX_DEVICE_IO_ALIAS_READ
FUNC_ALIAS(read, _read, ssize_t);
#endif

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	/* TODO: create  zvfs_select() and dispatch to subsystems based on file type */
	return zsock_select(nfds, readfds, writefds, exceptfds, (struct zsock_timeval *)timeout);
}

ssize_t write(int fd, const void *buf, size_t sz)
{
	return zvfs_write(fd, buf, sz);
}
#ifdef CONFIG_POSIX_DEVICE_IO_ALIAS_WRITE
FUNC_ALIAS(write, _write, ssize_t);
#endif
