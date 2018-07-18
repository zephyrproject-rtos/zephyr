/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
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

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <net/tls_credentials.h>
#include "ca_certificate.h"
#endif

#define sleep(x) k_sleep(x * 1000)

#endif

/* This URL is parsed in-place, so buffer must be non-const. */
static char download_url[] =
#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
    "http://archive.ubuntu.com/ubuntu/dists/xenial/main/installer-amd64/current/images/hd-media/vmlinuz";
#else
    "https://www.7-zip.org/a/7z1805.exe";
#endif /* !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) */
/* Quick testing. */
/*    "http://google.com/foo";*/

/* print("".join(["\\x%02x" % x for x in list(binascii.unhexlify("hash"))])) */
static uint8_t download_hash[32] =
#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
"\x33\x7c\x37\xd7\xec\x00\x34\x84\x14\x22\x4b\xaa\x6b\xdb\x2d\x43\xf2\xa3\x4e\xf5\x67\x6b\xaf\xcd\xca\xd9\x16\xf1\x48\xb5\xb3\x17";
#else
"\x64\x7a\x9a\x62\x11\x62\xcd\x7a\x50\x08\x93\x4a\x08\xe2\x3f\xf7\xc1\x13\x5d\x6f\x12\x61\x68\x9f\xd9\x54\xaa\x17\xd5\x0f\x97\x29";
#endif /* !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) */

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

ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);
		if (out_len < 0) {
			return out_len;
		}
		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

int skip_headers(int sock)
{
	int state = 0;

	while (1) {
		char c;
		int st;

		st = recv(sock, &c, 1, 0);
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

void download(struct addrinfo *ai, bool is_tls)
{
	int sock;

	cur_bytes = 0;

	if (is_tls) {
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		sock = socket(ai->ai_family, ai->ai_socktype, IPPROTO_TLS_1_2);
# else
		printf("TLS not supported\n");
		return;
#endif
	} else {
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	}

	CHECK(sock);
	printf("sock = %d\n", sock);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	if (is_tls) {
		sec_tag_t sec_tag_opt[] = {
			CA_CERTIFICATE_TAG,
		};
		CHECK(setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
				 sec_tag_opt, sizeof(sec_tag_opt)));

		CHECK(setsockopt(sock, SOL_TLS, TLS_HOSTNAME,
				 host, strlen(host) + 1));
	}
#endif

	CHECK(connect(sock, ai->ai_addr, ai->ai_addrlen));
	sendall(sock, "GET /", SSTRLEN("GET /"));
	sendall(sock, uri_path, strlen(uri_path));
	sendall(sock, " HTTP/1.0\r\n", SSTRLEN(" HTTP/1.0\r\n"));
	sendall(sock, "Host: ", SSTRLEN("Host: "));
	sendall(sock, host, strlen(host));
	sendall(sock, "\r\n\r\n", SSTRLEN("\r\n\r\n"));

	if (skip_headers(sock) <= 0) {
		printf("EOF or error in response headers\n");
		goto error;
	}

	mbedtls_md_starts(&hash_ctx);

	while (1) {
		int len = recv(sock, response, sizeof(response) - 1, 0);

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
	(void)close(sock);
}

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st;
	char *p;
	unsigned int total_bytes = 0;
	int resolve_attempts = 10;
	bool is_tls = false;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
			   ca_certificate, sizeof(ca_certificate));
#endif

	setbuf(stdout, NULL);

	if (strncmp(download_url, "http://", SSTRLEN("http://")) == 0) {
		port = "80";
		p = download_url + SSTRLEN("http://");
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	} else if (strncmp(download_url, "https://",
		   SSTRLEN("https://")) == 0) {
		is_tls = true;
		port = "443";
		p = download_url + SSTRLEN("https://");
#endif /* defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) */
	} else {
		fatal("Only http: "
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		      "and https: "
#endif
		      "URLs are supported");
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
		       (is_tls ? "s" : ""), host, port, uri_path);

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
		download(res, is_tls);

		total_bytes += cur_bytes;
		printf("Total downloaded so far: %uMB\n", total_bytes / (1024 * 1024));

		sleep(3);
	}

	mbedtls_md_free(&hash_ctx);

	return 0;
}
