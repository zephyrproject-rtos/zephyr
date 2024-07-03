/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "mbedtls/md.h"

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#endif

#if defined(__ZEPHYR__)

#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <zephyr/net/tls_credentials.h>
#include "ca_certificate.h"
#endif

#define sleep(x) k_sleep(K_MSEC((x) * MSEC_PER_SEC))

#endif

#define bytes2KiB(Bytes)	(Bytes / (1024u))
#define bytes2MiB(Bytes)	(Bytes / (1024u * 1024u))

#if defined(CONFIG_SAMPLE_BIG_HTTP_DL_MAX_URL_LENGTH)
#define MAX_URL_LENGTH CONFIG_SAMPLE_BIG_HTTP_DL_MAX_URL_LENGTH
#else
#define MAX_URL_LENGTH 256
#endif

#if defined(CONFIG_SAMPLE_BIG_HTTP_DL_NUM_ITER)
#define NUM_ITER CONFIG_SAMPLE_BIG_HTTP_DL_NUM_ITER
#else
#define NUM_ITER 0
#endif

/* This URL is parsed in-place, so buffer must be non-const. */
static char download_url[MAX_URL_LENGTH] =
#if defined(CONFIG_SAMPLE_BIG_HTTP_DL_URL)
    CONFIG_SAMPLE_BIG_HTTP_DL_URL;
#else
    "http://archive.ubuntu.com/ubuntu/dists/xenial/main/installer-amd64/current/images/hd-media/vmlinuz";
#endif /* defined(CONFIG_SAMPLE_BIG_HTTP_DL_URL) */
/* Quick testing. */
/*    "http://google.com/foo";*/

/* print("".join(["\\x%02x" % x for x in list(binascii.unhexlify("hash"))])) */
static uint8_t download_hash[32] =
#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
"\x33\x7c\x37\xd7\xec\x00\x34\x84\x14\x22\x4b\xaa\x6b\xdb\x2d\x43\xf2\xa3\x4e\xf5\x67\x6b\xaf\xcd\xca\xd9\x16\xf1\x48\xb5\xb3\x17";
#else
"\x3a\x07\x55\xdd\x1c\xfa\xb7\x1a\x24\xdd\x96\xdf\x34\x98\xc2\x9c"
"\xd0\xac\xd1\x3b\x04\xf3\xd0\x8b\xf9\x33\xe8\x12\x86\xdb\x80\x2c";
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

static int parse_status(bool *redirect)
{
	char *ptr;
	int code;

	ptr = strstr(response, "HTTP");
	if (ptr == NULL) {
		return -1;
	}

	ptr = strstr(response, " ");
	if (ptr == NULL) {
		return -1;
	}

	ptr++;

	code = atoi(ptr);
	if (code >= 300 && code < 400) {
		*redirect = true;
	}

	return 0;
}

static int parse_header(bool *location_found)
{
	char *ptr;

	ptr = strstr(response, ":");
	if (ptr == NULL) {
		return 0;
	}

	*ptr = '\0';
	ptr = response;

	while (*ptr != '\0') {
		*ptr = tolower(*ptr);
		ptr++;
	}

	if (strcmp(response, "location") != 0) {
		return 0;
	}

	/* Skip whitespace */
	while (*(++ptr) == ' ') {
		;
	}

	strncpy(download_url, ptr, sizeof(download_url));
	download_url[sizeof(download_url) - 1] = '\0';

	/* Trim LF. */
	ptr = strstr(download_url, "\n");
	if (ptr == NULL) {
		printf("Redirect URL too long or malformed\n");
		return -1;
	}

	*ptr = '\0';

	/* Trim CR if present. */
	ptr = strstr(download_url, "\r");
	if (ptr != NULL) {
		*ptr = '\0';
	}

	*location_found = true;

	return 0;
}

int skip_headers(int sock, bool *redirect)
{
	int state = 0;
	int i = 0;
	bool status_line = true;
	bool redirect_code = false;
	bool location_found = false;

	while (1) {
		int st;

		st = recv(sock, response + i, 1, 0);
		if (st <= 0) {
			return st;
		}

		if (state == 0 && response[i] == '\r') {
			state++;
		} else if ((state == 0 || state == 1) && response[i] == '\n') {
			state = 2;
			response[i + 1] = '\0';
			i = 0;

			if (status_line) {
				if (parse_status(&redirect_code) < 0) {
					return -1;
				}

				status_line = false;
			} else {
				if (parse_header(&location_found) < 0) {
					return -1;
				}
			}

			continue;
		} else if (state == 2 && response[i] == '\r') {
			state++;
		} else if ((state == 2 || state == 3) && response[i] == '\n') {
			break;
		} else {
			state = 0;
		}

		i++;
		if (i >= sizeof(response) - 1) {
			i = 0;
		}
	}

	if (redirect_code && location_found) {
		*redirect = true;
	}

	return 1;
}

void print_hex(const unsigned char *p, int len)
{
	while (len--) {
		printf("%02x", *p++);
	}
}

bool download(struct addrinfo *ai, bool is_tls, bool *redirect)
{
	int sock;
	struct timeval timeout = {
		.tv_sec = 5
	};

	cur_bytes = 0U;
	*redirect = false;

	if (is_tls) {
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		sock = socket(ai->ai_family, ai->ai_socktype, IPPROTO_TLS_1_2);
# else
		printf("TLS not supported\n");
		return false;
#endif
	} else {
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	}

	CHECK(sock);
	printf("sock = %d\n", sock);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	if (is_tls) {
		sec_tag_t sec_tag_opt[ARRAY_SIZE(ca_certificates)];

		for (int i = 0; i < ARRAY_SIZE(ca_certificates); i++) {
			sec_tag_opt[i] = CA_CERTIFICATE_TAG + i;
		};

		CHECK(setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
				 sec_tag_opt, sizeof(sec_tag_opt)));

		CHECK(setsockopt(sock, SOL_TLS, TLS_HOSTNAME,
				 host, strlen(host) + 1));
	}
#endif

	CHECK(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout,
			 sizeof(timeout)));
	CHECK(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout,
			 sizeof(timeout)));

	CHECK(connect(sock, ai->ai_addr, ai->ai_addrlen));
	sendall(sock, "GET /", SSTRLEN("GET /"));
	sendall(sock, uri_path, strlen(uri_path));
	sendall(sock, " HTTP/1.0\r\n", SSTRLEN(" HTTP/1.0\r\n"));
	sendall(sock, "Host: ", SSTRLEN("Host: "));
	sendall(sock, host, strlen(host));
	sendall(sock, "\r\n\r\n", SSTRLEN("\r\n\r\n"));

	if (skip_headers(sock, redirect) <= 0) {
		printf("EOF or error in response headers\n");
		goto error;
	}

	if (*redirect) {
		printf("Server requested redirection to %s\n", download_url);
		goto error;
	}

	mbedtls_md_starts(&hash_ctx);

	while (1) {
		int len = recv(sock, response, sizeof(response) - 1, 0);

		if (len < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				printf("Timeout on reading response\n");
			} else {
				printf("Error reading response\n");
			}

			goto error;
		}

		if (len == 0) {
			break;
		}

		mbedtls_md_update(&hash_ctx, response, len);

		cur_bytes += len;
		printf("Download progress: %u Bytes; %u KiB; %u MiB\r",
			cur_bytes, bytes2KiB(cur_bytes), bytes2MiB(cur_bytes));

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

	return redirect;
}

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st;
	char *p;
	unsigned int total_bytes = 0U;
	int resolve_attempts = 10;
	bool is_tls = false;
	unsigned int num_iterations = NUM_ITER;
	bool redirect = false;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	for (int i = 0; i < ARRAY_SIZE(ca_certificates); i++) {
		tls_credential_add(CA_CERTIFICATE_TAG + i,
				   TLS_CREDENTIAL_CA_CERTIFICATE,
				   ca_certificates[i],
				   strlen(ca_certificates[i]) + 1);
	}
#endif

	setbuf(stdout, NULL);

redirect:
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

	const uint32_t total_iterations = num_iterations;
	uint32_t current_iteration = 1;
	do {
		if (total_iterations == 0) {
			printf("\nIteration %u of INF\n", current_iteration);
		} else {
			printf("\nIteration %u of %u:\n",
				current_iteration, total_iterations);
		}

		download(res, is_tls, &redirect);
		if (redirect) {
			goto redirect;
		}

		total_bytes += cur_bytes;
		printf("Total downloaded so far: %u MiB\n",
			bytes2MiB(total_bytes));

		sleep(3);
		current_iteration++;
	} while (--num_iterations != 0);

	printf("Finished downloading.\n");

	mbedtls_md_free(&hash_ctx);
	return 0;
}
