/*  Minimal TLS server.
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#include <zephyr.h>
#include <stdio.h>

#include <errno.h>
#include <misc/printk.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#define mbedtls_time_t       time_t
#define MBEDTLS_EXIT_SUCCESS EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE
#endif

#include <string.h>
#include <net/net_context.h>
#include <net/net_if.h>
#include "config.h"
#include "ssl_utils.h"
#include "test_certs.h"

#include "mbedtls/ssl_cookie.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

#if defined(MBEDTLS_DEBUG_C)
#include "mbedtls/debug.h"
#define DEBUG_THRESHOLD 0
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
static unsigned char heap[12000];
#endif

/*
 * Hardcoded values for server host and port
 */

static const char *pers = "tls_server";

#if defined(CONFIG_NET_IPV6)
static struct in6_addr server_addr;
#else
static struct in_addr server_addr;
#endif

struct parsed_url {
	const char *url;
	uint16_t url_len;
};

#define HTTP_RESPONSE \
	"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
	"<h2>Zephyr TLS Test Server</h2>\r\n" \
	"<p>Successful connection</p>\r\n"

#define HTTP_NOT_FOUND \
	"HTTP/1.0 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n" \
	"<h2>Zephyr TLS page not found </h2>\r\n" \
	"<p>Successful connection</p>\r\n"

static void my_debug(void *ctx, int level,
		     const char *file, int line, const char *str)
{
	const char *p, *basename;

	ARG_UNUSED(ctx);

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}

	}

	mbedtls_printf("%s:%04d: |%d| %s", basename, line, level, str);
}

static int entropy_source(void *data, unsigned char *output, size_t len,
			  size_t *olen)
{
	uint32_t seed;

	ARG_UNUSED(data);

	seed = sys_rand32_get();

	if (len > sizeof(seed)) {
		len = sizeof(seed);
	}

	memcpy(output, &seed, len);

	*olen = len;
	return 0;
}

static
int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct parsed_url *req = (struct parsed_url *)parser->data;

	req->url = at;
	req->url_len = length;

	return 0;
}

static unsigned char payload[256];

void https_server(void)
{
	struct ssl_context ctx;
	struct parsed_url request;
	int ret, len = 0;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt srvcert;
	mbedtls_pk_context pkey;

	mbedtls_platform_set_printf(printk);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
	mbedtls_memory_buffer_alloc_init(heap, sizeof(heap));
#endif

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(DEBUG_THRESHOLD);
	mbedtls_ssl_conf_dbg(&conf, my_debug, NULL);
#endif
	mbedtls_x509_crt_init(&srvcert);
	mbedtls_pk_init(&pkey);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	/*
	 * 1. Load the certificates and private RSA key
	 */

	ret = mbedtls_x509_crt_parse(&srvcert, rsa_example_cert_der,
				     rsa_example_cert_der_len);
	if (ret != 0) {
		mbedtls_printf(" failed\n  !"
			       "  mbedtls_x509_crt_parse returned %d\n\n", ret);
		goto exit;
	}

	ret = mbedtls_pk_parse_key(&pkey, rsa_example_keypair_der,
				   rsa_example_keypair_der_len, NULL, 0);
	if (ret != 0) {
		mbedtls_printf(" failed\n  !"
			       "  mbedtls_pk_parse_key returned %d\n\n", ret);
		goto exit;
	}

	/*
	 * 3. Seed the RNG
	 */

	mbedtls_entropy_add_source(&entropy, entropy_source, NULL,
				   MBEDTLS_ENTROPY_MAX_GATHER,
				   MBEDTLS_ENTROPY_SOURCE_STRONG);

	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				    (const unsigned char *)pers, strlen(pers));
	if (ret != 0) {
		mbedtls_printf(" failed\n  !"
			       " mbedtls_ctr_drbg_seed returned %d\n", ret);
		goto exit;
	}

	/*
	 * 4. Setup stuff
	 */
	ret = mbedtls_ssl_config_defaults(&conf,
					  MBEDTLS_SSL_IS_SERVER,
					  MBEDTLS_SSL_TRANSPORT_STREAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		mbedtls_printf(" failed\n  !"
			       " mbedtls_ssl_config_defaults returned %d\n\n",
			       ret);
		goto exit;
	}

	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_dbg(&conf, my_debug, NULL);

	mbedtls_ssl_conf_ca_chain(&conf, srvcert.next, NULL);
	ret = mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey);
	if (ret != 0) {
		mbedtls_printf(" failed\n  !"
			       " mbedtls_ssl_conf_own_cert returned %d\n\n",
			       ret);
		goto exit;
	}

	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0) {
		mbedtls_printf(" failed\n  !"
			       " mbedtls_ssl_setup returned %d\n\n", ret);
		goto exit;
	}

	/*
	 * 3. Wait until a client connects
	 */
	ret = ssl_init(&ctx, &server_addr);
	if (ret != 0) {
		mbedtls_printf(" failed\n  ! ssl_init returned %d\n\n", ret);
		goto exit;

	}

	/*
	 * 4. Prepare http parser
	 */
	http_parser_init(&ctx.parser, HTTP_REQUEST);
	http_parser_settings_init(&ctx.parser_settings);
	ctx.parser.data = &request;
	ctx.parser_settings.on_url = on_url;

	mbedtls_printf("Zephyr HTTPS Server\n");
	mbedtls_printf("Address: %s, port: %d\n", ZEPHYR_ADDR, SERVER_PORT);
reset:
	mbedtls_ssl_session_reset(&ssl);
	mbedtls_ssl_set_bio(&ssl, &ctx, ssl_tx, ssl_rx, NULL);
	/*
	 * 5. Handshake
	 */
	do {
		ret = mbedtls_ssl_handshake(&ssl);
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret < 0) {
				mbedtls_printf(" failed\n  !"
					       " mbedtls_ssl_handshake returned %d\n\n",
					       ret);
				goto reset;
			}
		}
	} while (ret != 0);

	/*
	 * 6. Read the HTTPS Request
	 */
	mbedtls_printf("Read HTTPS request\n");
	do {
		len = sizeof(payload) - 1;
		memset(payload, 0, sizeof(payload));
		ret = mbedtls_ssl_read(&ssl, payload, len);

		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}

		if (ret <= 0) {
			switch (ret) {
			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				mbedtls_printf(" connection was"
					       " closed gracefully\n");
				goto close;

			case MBEDTLS_ERR_NET_CONN_RESET:
				mbedtls_printf(" connection was"
					       " reset by peer\n");
				break;

			default:
				mbedtls_printf
				    (" mbedtls_ssl_read returned -0x%x\n",
				     -ret);
				break;
			}

			break;
		}

		len = ret;
		ret = http_parser_execute(&ctx.parser, &ctx.parser_settings,
					  payload, len);
		if (ret < 0) {
		}
	} while (ret < 0);

	/*
	 * 7. Write the Response
	 */
	mbedtls_printf("Write HTTPS response\n");

	if (!strncmp("/index.html", request.url, request.url_len)) {
		len = snprintf((char *)payload, sizeof(payload),
				HTTP_RESPONSE);
	} else {

		len = snprintf((char *)payload, sizeof(payload),
				HTTP_NOT_FOUND);
	}

	do {
		ret = mbedtls_ssl_write(&ssl, payload, len);
		if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
			mbedtls_printf(" failed\n  !"
				       " peer closed the connection\n");
			goto reset;
		}

		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret < 0) {
				mbedtls_printf(" failed\n  !"
					       " mbedtls_ssl_write"
					       " returned %d\n", ret);
				goto exit;
			}
		}
	} while (ret <= 0);

close:

	mbedtls_ssl_close_notify(&ssl);
	ret = 0;
	goto reset;

exit:
#ifdef MBEDTLS_ERROR_C
	if (ret != 0) {
		mbedtls_strerror(ret, payload, 100);
		mbedtls_printf("Last error was: %d - %s\n", ret, payload);
	}
#endif

	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
}

#define STACK_SIZE		8192
uint8_t stack[STACK_SIZE];

static inline int init_app(void)
{
#if defined(CONFIG_NET_IPV6)
	if (net_addr_pton(AF_INET6, ZEPHYR_ADDR, &server_addr) < 0) {
		mbedtls_printf("Invalid IPv6 address %s", ZEPHYR_ADDR);
	}

	if (!net_if_ipv6_addr_add(net_if_get_default(), &server_addr,
				  NET_ADDR_MANUAL, 0)) {
		return -EIO;
	}
#else
	if (net_addr_pton(AF_INET, ZEPHYR_ADDR, &server_addr) < 0) {
		mbedtls_printf("Invalid IPv4 address %s", ZEPHYR_ADDR);
	}

	if (!net_if_ipv4_addr_add(net_if_get_default(), &server_addr,
				  NET_ADDR_MANUAL, 0)) {
		return -EIO;
	}
#endif

	return 0;
}

void https_server_start(void)
{
	if (init_app() != 0) {
		printk("Cannot initialize network\n");
		return;
	}

	k_thread_spawn(stack, STACK_SIZE, (k_thread_entry_t) https_server,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);

}
