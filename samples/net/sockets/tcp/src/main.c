/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TCP Sample for TTCN-3 based Sanity Check */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <zephyr/net/socket.h>

#include <zephyr/data/json.h>
#include <tp.h>

#define UDP_PORT 4242

#define perror(fmt, args...)						\
do {									\
	printf("Error: " fmt "(): %s\n" ## args, strerror(errno));	\
	exit(errno);							\
} while (0)								\

/*
 * This application is used together with the TTCN-3 based sanity check
 * to validate the functionality of the TCP.
 *
 * samples/net/sockets/tcp/README.rst
 *
 * Eventually UDP based test protocol might be terminated in the user space
 * (see udp() below), but at the moment it's just a dummy loop
 * to keep the sample running in order to execute TTCN-3 TCP sanity check.
 */
int main(void)
{
	while (true) {
		k_sleep(K_SECONDS(1));
	}
	return 0;
}

void udp(void)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0) {
		perror("socket");
	}

	{
		struct sockaddr_in sin;

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
		sin.sin_port = htons(UDP_PORT);

		if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			perror("bind");
		}

		printf("Listening on UDP port %d\n", UDP_PORT);
	}

	{
#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif
		char *buf = malloc(BUF_SIZE);

		if (buf == NULL) {
			perror("malloc");
		}

		for (;;) {
			ssize_t len = recv(fd, buf, BUF_SIZE, 0);

			if (len < 0) {
				perror("recv");
			}

			printf("Received %ld bytes\n", (long)len);
		}
	}
}
