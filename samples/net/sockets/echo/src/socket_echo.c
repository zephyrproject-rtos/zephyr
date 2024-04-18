/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifndef __ZEPHYR__

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#else

#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>

#endif

#define BIND_PORT 4242

int main(void)
{
	int opt;
	socklen_t optlen = sizeof(int);
	int serv, ret;
	struct sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(BIND_PORT),
	};
	static int counter;

	serv = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (serv < 0) {
		printf("error: socket: %d\n", errno);
		exit(1);
	}

	ret = getsockopt(serv, IPPROTO_IPV6, IPV6_V6ONLY, &opt, &optlen);
	if (ret == 0) {
		if (opt) {
			printf("IPV6_V6ONLY option is on, turning it off.\n");

			opt = 0;
			ret = setsockopt(serv, IPPROTO_IPV6, IPV6_V6ONLY,
					 &opt, optlen);
			if (ret < 0) {
				printf("Cannot turn off IPV6_V6ONLY option\n");
			} else {
				printf("Sharing same socket between IPv6 and IPv4\n");
			}
		}
	}

	if (bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
		printf("error: bind: %d\n", errno);
		exit(1);
	}

	if (listen(serv, 5) < 0) {
		printf("error: listen: %d\n", errno);
		exit(1);
	}

	printf("Single-threaded TCP echo server waits for a connection on "
	       "port %d...\n", BIND_PORT);

	while (1) {
		struct sockaddr_in6 client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		char addr_str[32];
		int client = accept(serv, (struct sockaddr *)&client_addr,
				    &client_addr_len);

		if (client < 0) {
			printf("error: accept: %d\n", errno);
			continue;
		}

		inet_ntop(client_addr.sin6_family, &client_addr.sin6_addr,
			  addr_str, sizeof(addr_str));
		printf("Connection #%d from %s\n", counter++, addr_str);

		while (1) {
			char buf[128], *p;
			int len = recv(client, buf, sizeof(buf), 0);
			int out_len;

			if (len <= 0) {
				if (len < 0) {
					printf("error: recv: %d\n", errno);
				}
				break;
			}

			p = buf;
			do {
				out_len = send(client, p, len, 0);
				if (out_len < 0) {
					printf("error: send: %d\n", errno);
					goto error;
				}
				p += out_len;
				len -= out_len;
			} while (len);
		}

error:
		close(client);
		printf("Connection from %s closed\n", addr_str);
	}
	return 0;
}
