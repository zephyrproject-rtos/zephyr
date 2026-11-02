/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Host side of the test, built with the host C library: the listening end of the socket that the
 * driver connects to.
 */

#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "peer_bottom.h"

int peer_listen(const char *path)
{
	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return -1;
	}

	(void)strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
	(void)unlink(path);

	if ((bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) || (listen(fd, 1) < 0)) {
		(void)close(fd);
		return -1;
	}

	return fd;
}

int peer_accept(int listen_fd)
{
	int fd;

	fd = accept(listen_fd, NULL, NULL);
	if (fd < 0) {
		return -1;
	}

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		(void)close(fd);
		return -1;
	}

	return fd;
}

void peer_stop_listening(int listen_fd, const char *path)
{
	(void)close(listen_fd);
	(void)unlink(path);
}
