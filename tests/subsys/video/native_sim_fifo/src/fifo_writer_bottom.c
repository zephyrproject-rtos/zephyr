/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Host (bottom) half of the native_sim host FIFO video source test suite.
 *
 * This file is built in the native simulator runner context: it uses the host C
 * library and must not include any Zephyr header. It plays the role the ffmpeg
 * process plays in a real setup, so that the tests need no external tool.
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <nsi_tracing.h>

#include "fifo_writer_bottom.h"

int video_fifo_test_open_writer(const char *path)
{
	int fd = open(path, O_WRONLY | O_NONBLOCK);

	if (fd < 0) {
		nsi_print_warning("Could not open %s for writing (%s)\n", path, strerror(errno));
		return -1;
	}

	return fd;
}

int video_fifo_test_write(int fd, const uint8_t *buf, size_t len)
{
	size_t offset = 0;

	while (offset < len) {
		ssize_t ret = write(fd, &buf[offset], len - offset);

		if (ret > 0) {
			offset += (size_t)ret;
			continue;
		}

		if ((ret < 0) && (errno == EINTR)) {
			continue;
		}

		/*
		 * Never retry on EAGAIN: the driver only drains the pipe while the
		 * simulated CPU runs, so spinning here would deadlock the simulation.
		 * The caller gets a short count and decides what to do about it.
		 */
		if ((ret < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
			nsi_print_warning("Error writing the video FIFO (%s)\n", strerror(errno));
		}

		break;
	}

	return (int)offset;
}

int video_fifo_test_close(int fd)
{
	if (close(fd) != 0) {
		nsi_print_warning("Could not close the video FIFO writer (%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}

int video_fifo_test_unlink(const char *path)
{
	if ((unlink(path) != 0) && (errno != ENOENT)) {
		nsi_print_warning("Could not remove %s (%s)\n", path, strerror(errno));
		return -1;
	}

	return 0;
}
