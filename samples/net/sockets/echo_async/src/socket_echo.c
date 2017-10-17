/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>

#ifndef __ZEPHYR__

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#else

#include <sys/fcntl.h>
#include <net/socket.h>
#include <kernel.h>
#include <net/net_app.h>

#endif

/* For Zephyr, keep max number of fd's in sync with max poll() capacity */
#ifdef CONFIG_NET_SOCKETS_POLL_MAX
#define NUM_FDS CONFIG_NET_SOCKETS_POLL_MAX
#else
#define NUM_FDS 5
#endif

/* Number of simultaneous client connections will be NUM_FDS be minus 2 */
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
	int res;
	static int counter;
	int serv4, serv6;
	struct sockaddr_in bind_addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(4242),
		.sin_addr = {
			.s_addr = htonl(INADDR_ANY),
		},
	};
	struct sockaddr_in6 bind_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(4242),
		.sin6_addr = IN6ADDR_ANY_INIT,
	};

	serv4 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	res = bind(serv4, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	if (res == -1) {
		printf("Cannot bind IPv4, errno: %d\n", errno);
	}

	serv6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	#ifdef IPV6_V6ONLY
	/* For Linux, we need to make socket IPv6-only to bind it to the
	 * same port as IPv4 socket above.
	 */
	int TRUE = 1;
	setsockopt(serv6, IPPROTO_IPV6, IPV6_V6ONLY, &TRUE, sizeof(TRUE));
	#endif
	res = bind(serv6, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	if (res == -1) {
		printf("Cannot bind IPv6, errno: %d\n", errno);
	}

	nonblock(serv4);
	nonblock(serv6);
	listen(serv4, 5);
	listen(serv6, 5);

	pollfds_add(serv4);
	pollfds_add(serv6);

	printf("Listening on port 4242...\n");

	while (1) {
		struct sockaddr_storage client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		char addr_str[32];

		res = poll(pollfds, pollnum, -1);
		if (res == -1) {
			printf("poll error: %d\n", errno);
			continue;
		}

		for (int i = 0; i < pollnum; i++) {
			if (!(pollfds[i].revents & POLLIN)) {
				continue;
			}
			int fd = pollfds[i].fd;
			if (i < 2) {
				/* If server socket */
				int client = accept(fd, (struct sockaddr *)&client_addr,
						    &client_addr_len);
				void *addr = &((struct sockaddr_in *)&client_addr)->sin_addr;

				inet_ntop(client_addr.ss_family, addr,
					  addr_str, sizeof(addr_str));
				printf("Connection #%d from %s fd=%d\n", counter++,
				       addr_str, client);
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
