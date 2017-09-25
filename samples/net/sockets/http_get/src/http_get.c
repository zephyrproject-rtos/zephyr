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
#include <netdb.h>

#else

#include <net/socket.h>
#include <kernel.h>
#include <net/net_app.h>

#endif

#define SSTRLEN(s) (sizeof(s) - 1)
#define CHECK(r) { if (r == -1) { printf("Error: " #r "\n"); } }

#define REQUEST "GET / HTTP/1.0\r\n\r\n"

static char response[1024];

void dump_addrinfo(const struct addrinfo *ai)
{
	printf("addrinfo @%p: fam=%d, socktype=%d, proto=%d, "
	       "addr_fam=%d, addr_port=%x\n",
	       ai, ai->ai_family, ai->ai_socktype, ai->ai_protocol,
	       ai->ai_addr->sa_family,
	       ((struct sockaddr_in *)ai->ai_addr)->sin_port);
}

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st, sock;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	st = getaddrinfo("google.com", "80", &hints, &res);
	printf("getaddrinfo status: %d\n", st);

	if (st != 0) {
		printf("Unable to resolve address, quitting\n");
		return 1;
	}

#if 0
	for (; res; res = res->ai_next) {
		dump_addrinfo(res);
	}
#endif

	dump_addrinfo(res);

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	CHECK(sock);
	printf("sock = %d\n", sock);
	CHECK(connect(sock, res->ai_addr, res->ai_addrlen));
	send(sock, REQUEST, SSTRLEN(REQUEST), 0);

	printf("Response:\n");

	while (1) {
		int len = recv(sock, response, sizeof(response) - 1, 0);

		if (len < 0) {
			printf("Error reading response\n");
			return 1;
		}

		if (len == 0) {
			break;
		}

		response[len] = 0;
		printf("%s", response);
	}

	return 0;
}
