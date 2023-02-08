/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

/* Generic read()/write() is available in POSIX config, so use it. */
#define READ(fd, buf, sz) read(fd, buf, sz)
#define WRITE(fd, buf, sz) write(fd, buf, sz)

#else

#include <fcntl.h>
#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>

/* Generic read()/write() are not defined, so use socket-specific recv(). */
#define READ(fd, buf, sz) recv(fd, buf, sz, 0)
#define WRITE(fd, buf, sz) send(fd, buf, sz, 0)

#endif

/* For Zephyr, keep max number of fd's in sync with max poll() capacity */
#ifdef CONFIG_NET_SOCKETS_POLL_MAX
#define NUM_FDS CONFIG_NET_SOCKETS_POLL_MAX
#else
#define NUM_FDS 5
#endif

#define BIND_PORT 4242

/* Number of simultaneous client connections will be NUM_FDS be minus 2 */
fd_set readfds;
fd_set workfds;
int fdnum;

#define fatal(msg, ...) { \
		printf("Error: " msg "\n", ##__VA_ARGS__); \
		exit(1); \
	}


static void setblocking(int fd, bool val)
{
	int fl, res;

	fl = fcntl(fd, F_GETFL, 0);
	if (fl == -1) {
		fatal("fcntl(F_GETFL): %d", errno);
	}

	if (val) {
		fl &= ~O_NONBLOCK;
	} else {
		fl |= O_NONBLOCK;
	}

	res = fcntl(fd, F_SETFL, fl);
	if (fl == -1) {
		fatal("fcntl(F_SETFL): %d", errno);
	}
}

int pollfds_add(int fd)
{
	FD_SET(fd, &readfds);

	if (fd >= fdnum) {
		fdnum = fd + 1;
	}

	return 0;
}

void pollfds_del(int fd)
{
	FD_CLR(fd, &readfds);
}

int main(void)
{
	int res;
	static int counter;
#if !defined(CONFIG_SOC_SERIES_CC32XX)
	int serv4;
	struct sockaddr_in bind_addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(BIND_PORT),
		.sin_addr = {
			.s_addr = htonl(INADDR_ANY),
		},
	};
#endif
	int serv6;
	struct sockaddr_in6 bind_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(BIND_PORT),
		.sin6_addr = IN6ADDR_ANY_INIT,
	};

	FD_ZERO(&readfds);
#if !defined(CONFIG_SOC_SERIES_CC32XX)
	serv4 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serv4 < 0) {
		printf("error: socket: %d\n", errno);
		exit(1);
	}

	res = bind(serv4, (struct sockaddr *)&bind_addr4, sizeof(bind_addr4));
	if (res == -1) {
		printf("Cannot bind IPv4, errno: %d\n", errno);
	}

	setblocking(serv4, false);
	listen(serv4, 5);
	pollfds_add(serv4);
#endif
	serv6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (serv6 < 0) {
		printf("error: socket(AF_INET6): %d\n", errno);
		exit(1);
	}
	#ifdef IPV6_V6ONLY
	/* For Linux, we need to make socket IPv6-only to bind it to the
	 * same port as IPv4 socket above.
	 */
	int TRUE = 1;
	res = setsockopt(serv6, IPPROTO_IPV6, IPV6_V6ONLY, &TRUE, sizeof(TRUE));
	if (res < 0) {
		printf("error: setsockopt: %d\n", errno);
		exit(1);
	}
	#endif
	res = bind(serv6, (struct sockaddr *)&bind_addr6, sizeof(bind_addr6));
	if (res == -1) {
		printf("Cannot bind IPv6, errno: %d\n", errno);
	}

	setblocking(serv6, false);
	listen(serv6, 5);
	pollfds_add(serv6);

	printf("Async select-based TCP echo server waits for connections on "
	       "port %d...\n", BIND_PORT);

	while (1) {
		struct sockaddr_storage client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		char addr_str[32];

		/* As select overwrites passed fd_set, we run it against
		 * temporary fd_set, by first copying a real fd_set into it.
		 */
		workfds = readfds;
		res = select(fdnum, &workfds, NULL, NULL, NULL);
		if (res == -1) {
			printf("select error: %d\n", errno);
			continue;
		}

		for (int i = 0; i < fdnum; i++) {
			if (!FD_ISSET(i, &workfds)) {
				continue;
			}
			int fd = i;
#if defined(CONFIG_SOC_SERIES_CC32XX)
			/*
			 * On TI CC32xx, the same port cannot be bound to two
			 * different sockets. Instead, the IPv6 socket is used
			 * to handle both IPv4 and IPv6 connections.
			 */
			if (fd == serv6) {
#else
			if (fd == serv4 || fd == serv6) {
#endif
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
					WRITE(client, msg, sizeof(msg) - 1);
					close(client);
				} else {
					setblocking(client, false);
				}
			} else {
				char buf[128];
				int len = READ(fd, buf, sizeof(buf));
				if (len <= 0) {
					if (len < 0) {
						printf("error: RECV: %d\n",
						       errno);
					}
error:
					pollfds_del(fd);
					close(fd);
					printf("Connection fd=%d closed\n", fd);
				} else {
					int out_len;
					const char *p;
					/* We implement semi-async server,
					 * where reads are async, but writes
					 * *can* be sync (blocking). Note that
					 * in majority of cases they expected
					 * to not block, but to be robust, we
					 * handle all possibilities.
					 */
					setblocking(fd, true);

					for (p = buf; len; len -= out_len) {
						out_len = WRITE(fd, p, len);
						if (out_len < 0) {
							printf("error: "
							       "WRITE: %d\n",
							       errno);
							goto error;
						}
						p += out_len;
					}

					setblocking(fd, false);
				}
			}
		}
	}
	return 0;
}
