/*  Minimal DTLS server.
 *  (Meant to be used with config-threadnet.h)
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
#include "udp.h"
#include "udp_cfg.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ssl_cookie.h"

#if defined(MBEDTLS_DEBUG_C)
#include "mbedtls/debug.h"
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
static unsigned char heap[20480];
#endif

#define DEBUG_THRESHOLD 0
/*
 * Hardcoded values for server host and port
 */

const char *pers = "dtsl_server";

#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
#define ECJPAKE_PW_SIZE 6
const unsigned char ecjpake_pw[ECJPAKE_PW_SIZE] = "passwd";
#endif

struct dtls_timing_context {
	u32_t snapshot;
	u32_t int_ms;
	u32_t fin_ms;
};

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

void dtls_timing_set_delay(void *data, u32_t int_ms, u32_t fin_ms)
{
	struct dtls_timing_context *ctx = (struct dtls_timing_context *)data;

	ctx->int_ms = int_ms;
	ctx->fin_ms = fin_ms;

	if (fin_ms != 0) {
		ctx->snapshot = k_uptime_get_32();
	}
}

int dtls_timing_get_delay(void *data)
{
	struct dtls_timing_context *ctx = (struct dtls_timing_context *)data;
	unsigned long elapsed_ms;

	if (ctx->fin_ms == 0) {
		return -1;
	}

	elapsed_ms = k_uptime_get_32() - ctx->snapshot;

	if (elapsed_ms >= ctx->fin_ms) {
		return 2;
	}

	if (elapsed_ms >= ctx->int_ms) {
		return 1;
	}

	return 0;
}

static int entropy_source(void *data, unsigned char *output, size_t len,
			  size_t *olen)
{
	u32_t seed;

	ARG_UNUSED(data);

	seed = sys_rand32_get();

	if (len > sizeof(seed)) {
		len = sizeof(seed);
	}

	memcpy(output, &seed, len);

	*olen = len;

	return 0;
}

unsigned char payload[256];

void dtls_server(void)
{
	int len, ret = 0;
	struct udp_context ctx;
	struct dtls_timing_context timer;

	mbedtls_ssl_cookie_ctx cookie_ctx;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;

	mbedtls_ctr_drbg_init(&ctr_drbg);

	mbedtls_platform_set_printf(printk);

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(DEBUG_THRESHOLD);
#endif

	/*
	 * Initialize and setup
	 */
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);

	mbedtls_printf("\n  . Seeding the random number generator...");
	mbedtls_entropy_init(&entropy);
	mbedtls_entropy_add_source(&entropy, entropy_source, NULL,
				   MBEDTLS_ENTROPY_MAX_GATHER,
				   MBEDTLS_ENTROPY_SOURCE_STRONG);

	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				    (const unsigned char *)pers, strlen(pers));
	if (ret != 0) {
		mbedtls_printf
		    (" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x\n",
		     -ret);
		goto exit;
	}

	mbedtls_printf(" ok\n");

	mbedtls_printf("  . Setting up the DTLS structure...");
	ret = mbedtls_ssl_config_defaults(&conf,
					  MBEDTLS_SSL_IS_SERVER,
					  MBEDTLS_SSL_TRANSPORT_DATAGRAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		mbedtls_printf(" failed! returned -0x%x\n\n", -ret);
		goto exit;
	}

/* Modify this to change the default timeouts for the DTLS handshake */
/*        mbedtls_ssl_conf_handshake_timeout( &conf, min, max ); */

	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_dbg(&conf, my_debug, NULL);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
	mbedtls_memory_buffer_alloc_init(heap, sizeof(heap));
#endif
	ret = mbedtls_ssl_cookie_setup(&cookie_ctx, mbedtls_ctr_drbg_random,
				       &ctr_drbg);
	if (ret != 0) {
		printf(" failed\n ! mbedtls_ssl_cookie_setup returned -0x%x\n",
		       -ret);
		goto exit;
	}

	mbedtls_ssl_conf_dtls_cookies(&conf, mbedtls_ssl_cookie_write,
				      mbedtls_ssl_cookie_check, &cookie_ctx);

	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0) {
		mbedtls_printf
		    (" failed\n  ! mbedtls_ssl_setup returned -0x%x\n", -ret);
		goto exit;
	}

	mbedtls_printf(" ok\n");

	/*
	 * Start the connection
	 */

	mbedtls_printf("  . Setting connection\n");

	ret = udp_init(&ctx);
	if (ret != 0) {
		mbedtls_printf(" failed! udp_init returned -0x%x\n", -ret);
		goto exit;
	}

	mbedtls_printf(" ok\n");

reset:
	mbedtls_ssl_session_reset(&ssl);

#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
	mbedtls_printf("  . Setting up ecjpake password ...");
	ret = mbedtls_ssl_set_hs_ecjpake_password(&ssl, ecjpake_pw,
						  ECJPAKE_PW_SIZE);
	if (ret != 0) {
		mbedtls_printf(" failed! set ecjpake password returned -%d\n\n",
			       -ret);
		goto exit;
	}
#endif
	mbedtls_printf(" ok\n");

	mbedtls_ssl_set_timer_cb(&ssl, &timer, dtls_timing_set_delay,
				 dtls_timing_get_delay);

	mbedtls_ssl_set_bio(&ssl, &ctx, udp_tx, udp_rx, NULL);

	/* For HelloVerifyRequest cookies */
	ctx.client_id = (char)ctx.remaining;

	ret = mbedtls_ssl_set_client_transport_id(
		&ssl, (unsigned char *)&ctx.client_id, sizeof(char));
	if (ret != 0) {
		mbedtls_printf(" failed\n  ! "
			       "mbedtls_ssl_set_client_transport_id()"
			       " returned -0x%x\n\n", -ret);
		goto exit;
	}

	mbedtls_printf("  . Performing the TLS handshake...");

	do {
		ret = mbedtls_ssl_handshake(&ssl);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		 ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
		mbedtls_printf(" hello verification requested\n");
		ret = 0;
		goto reset;
	}

	if (ret != 0) {
		mbedtls_printf
		    (" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n",
		     -ret);
		goto reset;
	}

	mbedtls_printf(" ok\n");

	do {
		/* Read the request */
		mbedtls_printf("  < Read from client:");
		len = sizeof(payload) - 1;
		memset(payload, 0, sizeof(payload));
		ret = mbedtls_ssl_read(&ssl, payload, len);

		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}

		if (ret <= 0) {
			switch (ret) {
			case MBEDTLS_ERR_SSL_TIMEOUT:
				mbedtls_printf(" timeout\n");
				goto reset;

			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				mbedtls_printf(" connection was closed"
					       " gracefully\n");
				ret = 0;
				goto close_notify;

			default:
				mbedtls_printf(" mbedtls_ssl_read"
					       " returned -0x%x\n", -ret);
				goto reset;
			}
		}

		len = ret;
		mbedtls_printf(" %d bytes read\n\n%s\n\n", len, payload);

		/* Write the response */
		mbedtls_printf("  > Write to client:");

		do {
			ret = mbedtls_ssl_write(&ssl, payload, len);
		} while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
			 ret == MBEDTLS_ERR_SSL_WANT_WRITE);

		if (ret < 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_write"
				       " returned -%d\n\n", -ret);
			goto exit;
		}

		len = ret;
		mbedtls_printf(" %d bytes written\n\n%s\n\n", len, payload);
	} while (1);

close_notify:
	mbedtls_printf("  . Closing the connection...");
	/* No error checking, the connection might be closed already */
	do {
		ret = mbedtls_ssl_close_notify(&ssl);
	} while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
	ret = 0;
	mbedtls_printf(" done\n");
	goto reset;

exit:
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
}

#define STACK_SIZE		8192
K_THREAD_STACK_DEFINE(stack, STACK_SIZE);
static struct k_thread dtls_thread;

static inline int init_app(void)
{
#if defined(CONFIG_NET_IPV6)
#if defined(CONFIG_NET_APP_MY_IPV6_ADDR)
	if (net_addr_pton(AF_INET6, CONFIG_NET_APP_MY_IPV6_ADDR,
			  &server_addr) < 0) {
		mbedtls_printf("Invalid IPv6 address %s",
			       CONFIG_NET_APP_MY_IPV6_ADDR);
	}
#endif
	if (!net_if_ipv6_addr_add(net_if_get_default(), &server_addr,
				  NET_ADDR_MANUAL, 0)) {
		return -EIO;
	}

	net_if_ipv6_maddr_add(net_if_get_default(), &mcast_addr);

#else
#if defined(CONFIG_NET_APP_MY_IPV4_ADDR)
	if (net_addr_pton(AF_INET, CONFIG_NET_APP_MY_IPV4_ADDR,
			  &server_addr) < 0) {
		mbedtls_printf("Invalid IPv4 address %s",
			       CONFIG_NET_APP_MY_IPV4_ADDR);
	}
#endif

	if (!net_if_ipv4_addr_add(net_if_get_default(), &server_addr,
				  NET_ADDR_MANUAL, 0)) {
		return -EIO;
	}
#endif
	return 0;
}

void main(void)
{
	if (init_app() != 0) {
		printf("Cannot initialize network\n");
		return;
	}

	k_thread_create(&dtls_thread, stack, STACK_SIZE,
			(k_thread_entry_t) dtls_server,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
