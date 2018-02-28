/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbedtls/md.h"

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
#define sleep(x) k_sleep(x * 1000)

#endif

#include <net/tls_conf.h>
#include <net/zstream.h>
#include <net/zstream_tls.h>

/* This URL is parsed in-place, so buffer must be non-const. */
static char download_url[] =
    /*"http://archive.ubuntu.com/ubuntu/dists/xenial/main/installer-amd64/current/images/hd-media/vmlinuz";*/
    "https://ftp.gnu.org/gnu/tar/tar-1.13.tar";
/* Quick testing. */
/*    "http://google.com/foo";*/

/* To generate: */
/* print("".join(["\\x%02x" % x for x in list(binascii.unhexlify("hash"))])) */
static uint8_t download_hash[32] =
    /*"\x33\x7c\x37\xd7\xec\x00\x34\x84\x14\x22\x4b\xaa\x6b\xdb\x2d\x43\xf2\xa3\x4e\xf5\x67\x6b\xaf\xcd\xca\xd9\x16\xf1\x48\xb5\xb3\x17";*/
    "\xbe\x12\xfe\x40\xe1\xb2\x02\xd7\x0c\x45\x5d\x78\x4f\xbe\xc8\xcd\xb3\x38\xfe\x01\x1a\xb9\xe8\x62\x95\x81\x35\x68\x90\x6f\x60\x73";

#define SSTRLEN(s) (sizeof(s) - 1)
#define CHECK(r) { if (r == -1) { printf("Error: " #r "\n"); exit(1); } }

const char *host;
const char *port;
const char *uri_path = "";
static char response[1024];
static char response_hash[32];
mbedtls_md_context_t hash_ctx;
const mbedtls_md_info_t *hash_info;
unsigned int cur_bytes;

void dump_addrinfo(const struct addrinfo *ai)
{
	printf("addrinfo @%p: ai_family=%d, ai_socktype=%d, ai_protocol=%d, "
	       "sa_family=%d, sin_port=%x\n",
	       ai, ai->ai_family, ai->ai_socktype, ai->ai_protocol,
	       ai->ai_addr->sa_family,
	       ((struct sockaddr_in *)ai->ai_addr)->sin_port);
}

void fatal(const char *msg)
{
	printf("Error: %s\n", msg);
	exit(1);
}

ssize_t sendall(struct zstream *stream, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = zstream_write(stream, buf, len);
		if (out_len < 0) {
			return out_len;
		}
		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

int skip_headers(struct zstream *stream)
{
	int state = 0;

	while (1) {
		char c;
		int st;

		st = zstream_read(stream, &c, 1);
		if (st <= 0) {
			return st;
		}

		if (state == 0 && c == '\r') {
			state++;
		} else if (state == 1 && c == '\n') {
			state++;
		} else if (state == 2 && c == '\r') {
			state++;
		} else if (state == 3 && c == '\n') {
			break;
		} else {
			state = 0;
		}
	}

	return 1;
}

void print_hex(const unsigned char *p, int len)
{
	while (len--) {
		printf("%02x", *p++);
	}
}

void download(struct addrinfo *ai, mbedtls_ssl_config *tls_conf)
{
	int sock;
	struct zstream_sock stream_sock;
	struct zstream_tls stream_tls;
	struct zstream *stream;

	cur_bytes = 0;

	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	CHECK(sock);
	printf("sock = %d\n", sock);
	CHECK(connect(sock, ai->ai_addr, ai->ai_addrlen));

	zstream_sock_init(&stream_sock, sock);
	stream = (struct zstream *)&stream_sock;

	if (tls_conf) {
		zstream_tls_init(&stream_tls, stream, tls_conf, host);
		stream = (struct zstream *)&stream_tls;
	}

	sendall(stream, "GET /", SSTRLEN("GET /"));
	sendall(stream, uri_path, strlen(uri_path));
	sendall(stream, " HTTP/1.0\r\n", SSTRLEN(" HTTP/1.0\r\n"));
	sendall(stream, "Host: ", SSTRLEN("Host: "));
	sendall(stream, host, strlen(host));
	sendall(stream, "\r\n\r\n", SSTRLEN("\r\n\r\n"));
	zstream_flush(stream);

	if (skip_headers(stream) <= 0) {
		printf("EOF or error in response headers\n");
		goto error;
	}

	mbedtls_md_starts(&hash_ctx);

	while (1) {
		int len = zstream_read(stream, response, sizeof(response) - 1);

		if (len < 0) {
			printf("Error reading response\n");
			goto error;
		}

		if (len == 0) {
			break;
		}

		mbedtls_md_update(&hash_ctx, response, len);

		cur_bytes += len;
		printf("%u bytes\r", cur_bytes);

		response[len] = 0;
		/*printf("%s\n", response);*/
	}

	printf("\n");

	mbedtls_md_finish(&hash_ctx, response_hash);

	printf("Hash: ");
	print_hex(response_hash, mbedtls_md_get_size(hash_info));
	printf("\n");

	if (memcmp(response_hash, download_hash,
		   mbedtls_md_get_size(hash_info)) != 0) {
		printf("HASH MISMATCH!\n");
	}

error:
	(void)zstream_close(sock);
}

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st;
	char *p;
	unsigned int total_bytes = 0;
	int resolve_attempts = 10;
	mbedtls_ssl_config *tls_conf = NULL;

	setbuf(stdout, NULL);

	if (strncmp(download_url, "http://", SSTRLEN("http://")) == 0) {
		port = "80";
		p = download_url + SSTRLEN("http://");
	} else if (strncmp(download_url, "https://",
		   SSTRLEN("https://")) == 0) {
		if (ztls_get_tls_client_conf(&tls_conf) < 0) {
			printf("Unable to initialize TLS\n");
			return 1;
		}
		mbedtls_ssl_conf_authmode(tls_conf, MBEDTLS_SSL_VERIFY_NONE);
		printf("Warning: site certificate is not validated\n");
		port = "443";
		p = download_url + SSTRLEN("https://");
	} else {
		fatal("Only http: and https: URLs are supported");
	}

	/* Parse host part */
	host = p;
	while (*p && *p != ':' && *p != '/') {
		p++;
	}

	/* Store optional port part */
	if (*p == ':') {
		*p++ = 0;
		port = p;
	}

	/* Parse path part */
	while (*p && *p != '/') {
		p++;
	}

	if (*p == '/') {
		*p++ = 0;
		uri_path = p;
	}

	printf("Preparing HTTP GET request for http%s://%s:%s/%s\n",
	       (tls_conf ? "s" : ""), host, port, uri_path);

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	while (resolve_attempts--) {
		st = getaddrinfo(host, port, &hints, &res);

		if (st == 0) {
			break;
		}

		printf("getaddrinfo status: %d, retrying\n", st);
		sleep(2);
	}

	if (st != 0) {
		fatal("Unable to resolve address");
	}

	dump_addrinfo(res);

	hash_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!hash_info) {
		fatal("Unable to request hash type from mbedTLS");
	}

	mbedtls_md_init(&hash_ctx);
	if (mbedtls_md_setup(&hash_ctx, hash_info, 0) < 0) {
		fatal("Can't setup mbedTLS hash engine");
	}

	while (1) {
		download(res, tls_conf);

		total_bytes += cur_bytes;
		printf("Total downloaded so far: %uMB\n", total_bytes / (1024 * 1024));

		sleep(3);
	}

	mbedtls_md_free(&hash_ctx);

	return 0;
}
