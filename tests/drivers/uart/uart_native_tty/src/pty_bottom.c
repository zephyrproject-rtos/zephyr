/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Built in the native simulator runner context, with the host C library. */

#undef _XOPEN_SOURCE
/* For posix_openpt(), grantpt(), unlockpt() and ptsname() */
#define _XOPEN_SOURCE 600

#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pty_bottom.h"

int native_tty_test_open_pty(char *path, long size)
{
	int fd = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);

	if (fd < 0) {
		return -1;
	}

	if (grantpt(fd) || unlockpt(fd) || ptsname(fd) == NULL ||
	    snprintf(path, (size_t)size, "%s", ptsname(fd)) >= size) {
		close(fd);
		return -1;
	}

	return fd;
}

int native_tty_test_write(int fd, const char *data, long size)
{
	return write(fd, data, (size_t)size) == size ? 0 : -1;
}

void native_tty_test_wait_readable(int fd)
{
	struct pollfd pfd = {.fd = fd, .events = POLLIN};

	(void)poll(&pfd, 1, -1);
}

void native_tty_test_close(int fd)
{
	close(fd);
}
