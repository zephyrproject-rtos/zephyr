/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

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

#include <net/tls_conf.h>
#include <net/zstream.h>
#include <net/zstream_tls.h>

/* HTTP server to connect to */
#define HTTP_HOST "google.com"
/* Port to connect to, as string */
#define HTTP_PORT "443"
/* HTTP path to request */
#define HTTP_PATH "/"


#define SSTRLEN(s) (sizeof(s) - 1)
#define CHECK(r) { if (r == -1) { printf("Error: " #r "\n"); exit(1); } }

#define REQUEST "GET " HTTP_PATH " HTTP/1.0\r\nHost: " HTTP_HOST "\r\n\r\n"

int http_request(struct zstream *stream);

static char response[1024];

void dump_addrinfo(const struct addrinfo *ai)
{
	printf("addrinfo @%p: ai_family=%d, ai_socktype=%d, ai_protocol=%d, "
	       "sa_family=%d, sin_port=%x\n",
	       ai, ai->ai_family, ai->ai_socktype, ai->ai_protocol,
	       ai->ai_addr->sa_family,
	       ((struct sockaddr_in *)ai->ai_addr)->sin_port);
}

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st, sock;
	struct zstream_sock stream_sock;
	struct zstream_tls stream_tls;
	struct zstream *stream;
	mbedtls_ssl_config *tls_conf;

	if (ztls_get_tls_client_conf(&tls_conf) < 0) {
		printf("Unable to initialize TLS\n");
		return 1;
	}

	printf("Preparing HTTP GET request for http://" HTTP_HOST
	       ":" HTTP_PORT HTTP_PATH "\n");

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	st = getaddrinfo(HTTP_HOST, HTTP_PORT, &hints, &res);
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

	/* Wrap socket into a stream */
	zstream_sock_init(&stream_sock, sock);
	stream = (struct zstream *)&stream_sock;

	/* Wrap socket stream into a TLS stream */
	mbedtls_ssl_conf_authmode(tls_conf, MBEDTLS_SSL_VERIFY_NONE);
	printf("Warning: site certificate is not validated\n");

	st = zstream_tls_init(&stream_tls, stream, tls_conf, NULL);
	if (st < 0) {
		printf("Unable to create TLS connection\n");
		return 1;
	}
	stream = (struct zstream *)&stream_tls;

	return http_request(stream);
}

int http_request(struct zstream *stream)
{
	CHECK(zstream_write(stream, REQUEST, SSTRLEN(REQUEST)));
	CHECK(zstream_flush(stream));

	printf("Response:\n\n");

	while (1) {
		int len = zstream_read(stream, response, sizeof(response) - 1);

		if (len < 0) {
			printf("Error reading response\n");
			return 1;
		}

		if (len == 0) {
			break;
		}

		response[len] = 0;
		printf("%s\n", response);
	}

	(void)zstream_close(sock);

	return 0;
}
