/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#ifndef __ZEPHYR__

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define init_net()

#else

#include <sys/fcntl.h>
#include <net/socket.h>
#include <kernel.h>
#include <net/net_app.h>

void init_net(void)
{
	int ret = net_app_init("socket_echo", NET_APP_NEED_IPV4, K_SECONDS(3));

	if (ret < 0) {
		printf("Application init failed\n");
		k_panic();
	}
}

#endif

/* Number of simultaneous connections will be minus 1 */
#define NUM_FDS 4

struct pollfd pollfds[NUM_FDS];
int pollnum;

static void nonblock(int fd)
{
	int fl = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

int pollfds_add(int fd)
{
	int i;
	if (pollnum < NUM_FDS) {
		i = pollnum++;
	} else {
		for (i = 0; i < NUM_FDS; i++) {
			if (pollfds[i].fd < 0) {
				goto found;
			}
		}

		return -1;
	}

found:
	pollfds[i].fd = fd;
	pollfds[i].events = POLLIN;

	return 0;
}

void pollfds_del(int fd)
{
	for (int i = 0; i < pollnum; i++) {
		if (pollfds[i].fd == fd) {
			pollfds[i].fd = -1;
			break;
		}
	}
}

int main(void)
{
	int serv;
	struct sockaddr_in bind_addr;

	init_net();

	serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(4242);
	bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

	nonblock(serv);
	pollfds_add(serv);

	listen(serv, 5);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		char addr_str[32];

		poll(pollfds, pollnum, -1);
		for (int i = 0; i < pollnum; i++) {
			if (!(pollfds[i].revents & POLLIN)) {
				continue;
			}
			int fd = pollfds[i].fd;
			if (fd == serv) {
				int client = accept(serv, (struct sockaddr *)&client_addr,
						    &client_addr_len);
				inet_ntop(client_addr.sin_family, &client_addr.sin_addr,
					  addr_str, sizeof(addr_str));
				printf("Connection from %s fd=%d\n", addr_str, client);
				if (pollfds_add(client) < 0) {
					static char msg[] = "Too many connections\n";
					send(client, msg, sizeof(msg) - 1, 0);
					close(client);
				} else {
					nonblock(client);
				}
			} else {
				char buf[128];
				int len = recv(fd, buf, sizeof(buf), 0);
				if (len == 0) {
					pollfds_del(fd);
					close(fd);
					printf("Connection fd=%d closed\n", fd);
				} else {
					/* We assume this won't be short write, d'oh */
					send(fd, buf, len, 0);
				}
			}
		}
	}
}
