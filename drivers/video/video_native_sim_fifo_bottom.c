/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Host (bottom) half of the native simulator host FIFO video source.
 *
 * This file is built in the native simulator runner context: it uses the host C
 * library and must not include any Zephyr header.
 */

/* For F_SETPIPE_SZ and F_GETPIPE_SZ */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <nsi_errno.h>
#include <nsi_tracing.h>

#include "video_native_sim_fifo_bottom.h"

int video_nsi_fifo_default_path_bottom(const char *name, char *buf, size_t size)
{
	int len = snprintf(buf, size, "/tmp/zephyr-%s-%ld.fifo", name, (long)getpid());

	return ((len < 0) || ((size_t)len >= size)) ? -1 : 0;
}

void video_nsi_fifo_unlink_bottom(const char *path)
{
	(void)unlink(path);
}

int video_nsi_fifo_open_bottom(const char *path, bool *created)
{
	struct stat statbuf;
	int fd;
	int err;

	/*
	 * Create the FIFO unconditionally and validate what was actually opened
	 * afterwards: inspecting the path before opening it would leave a window for it
	 * to be replaced in between (time-of-check to time-of-use race).
	 */
	if (mkfifo(path, 0666) == 0) {
		*created = true;
	} else if (errno != EEXIST) {
		err = errno;
		nsi_print_warning("Could not create the FIFO %s (%s)\n", path, strerror(err));
		return -nsi_errno_to_mid(err);
	}

	/*
	 * Opening read-only and non-blocking succeeds even when no writer is attached,
	 * and guarantees that no later read() can stall the simulated system.
	 */
	fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		err = errno;
		nsi_print_warning("Could not open the FIFO %s (%s)\n", path, strerror(err));
		return -nsi_errno_to_mid(err);
	}

	if (fstat(fd, &statbuf) != 0) {
		err = errno;
		nsi_print_warning("Could not stat %s (%s)\n", path, strerror(err));
		(void)close(fd);
		return -nsi_errno_to_mid(err);
	}

	if (!S_ISFIFO(statbuf.st_mode)) {
		nsi_print_warning("%s exists but is not a FIFO, it cannot be used as video "
				  "source\n",
				  path);
		(void)close(fd);
		return -nsi_errno_to_mid(EEXIST);
	}

	return fd;
}

int video_nsi_fifo_grow_pipe_bottom(int fd, size_t size)
{
	int capacity = fcntl(fd, F_GETPIPE_SZ);

	if (capacity < 0) {
		return -nsi_errno_to_mid(errno);
	}

	/*
	 * An unprivileged process gets EPERM above /proc/sys/fs/pipe-max-size, 1 MiB by
	 * default, or once its pipes together reach /proc/sys/fs/pipe-user-pages-soft.
	 * The kernel rounds the size up to a power of two, so halving the request,
	 * rounded up, tries each smaller capacity until one is allowed.
	 */
	for (size_t req = size; req > (size_t)capacity; req = (req + 1) / 2) {
		int ret = fcntl(fd, F_SETPIPE_SZ, (int)req);

		if (ret >= 0) {
			return ret;
		}

		if (errno != EPERM) {
			return -nsi_errno_to_mid(errno);
		}
	}

	return capacity;
}

int video_nsi_fifo_read_bottom(int fd, uint8_t *buf, size_t frame_size, size_t *offset)
{
	while (*offset < frame_size) {
		ssize_t ret = read(fd, &buf[*offset], frame_size - *offset);

		if (ret > 0) {
			*offset += (size_t)ret;
		} else if (ret == 0) {
			/* All the writers closed the FIFO */
			return -nsi_errno_to_mid(EPIPE);
		} else if (errno != EINTR) {
			return -nsi_errno_to_mid(errno);
		}
	}

	return 0;
}
