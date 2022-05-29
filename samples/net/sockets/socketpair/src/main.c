/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MSG "Hello, Zephyr!"
#define TIMEOUT_MS 100

int main(int argc, char **argv)
{
	int r;
	int len;
	int fd[2];
	char buf[] = MSG;
	struct pollfd pfd;

	BUILD_ASSERT(sizeof(buf) < CONFIG_NET_SOCKETPAIR_BUFFER_SIZE);

	len = strlen(MSG);
	memset(buf, 0, sizeof(buf));

	r = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (r < 0) {
		perror("socketpair");
		r = EXIT_FAILURE;
		goto out;
	}

	r = write(fd[0], MSG, len);
	if (r < 0) {
		perror("write");
		r = EXIT_FAILURE;
		goto close_fds;
	}

	if (r != len) {
		printf("only wrote %d/%d bytes", r, len);
		r = EXIT_FAILURE;
		goto close_fds;
	}

	printf("wrote %s to fd %d\n", MSG, fd[0]);

	pfd.fd = fd[1];
	pfd.events = POLLIN;
	r = poll(&pfd, 1, TIMEOUT_MS);
	if (r <= 0) {
		perror("poll");
		r = EXIT_FAILURE;
		goto close_fds;
	}

	if ((pfd.revents & POLLIN) == 0) {
		printf("no POLLIN event on fd %d\n", fd[1]);
		r = EXIT_FAILURE;
		goto close_fds;
	}

	r = read(fd[1], buf, sizeof(buf));
	if (r < 0) {
		perror("read");
		r = EXIT_FAILURE;
		goto close_fds;
	}

	if (r != len) {
		printf("only read %d/%d bytes", r, len);
		r = EXIT_FAILURE;
		goto close_fds;
	}

	printf("read %s from fd %d\n", buf, fd[1]);

close_fds:
	if (close(fd[0]) < 0) {
		perror("close");
		r = EXIT_FAILURE;
		goto out;
	}

	if (close(fd[1]) < 0) {
		perror("close");
		r = EXIT_FAILURE;
		goto out;
	}

	r = EXIT_SUCCESS;

out:
	return r;
}
