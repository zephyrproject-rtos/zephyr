/*  Sample CoAP over DTLS client using mbedTLS.
 *  (Meant to be used with config-coap.h)
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
#include <net/buf.h>
#include <net/nbuf.h>
#include "udp.h"
#include "udp_cfg.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#include <net/zoap.h>

#if defined(MBEDTLS_DEBUG_C)
#include "mbedtls/debug.h"
#define DEBUG_THRESHOLD 0
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
static unsigned char heap[8192];
#endif

/*
 * Hardcoded values for server host and port
 */

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
const unsigned char psk[] = "passwd\0";
const char psk_id[] = "Client_identity\0";
#endif

const char *pers = "mini_client";
static unsigned char payload[128];

#define NUM_REPLIES 3
struct zoap_reply replies[NUM_REPLIES];

#define ZOAP_BUF_SIZE 128

NET_BUF_POOL_DEFINE(zoap_nbuf_pool, 4, 0, sizeof(struct net_nbuf), NULL);
NET_BUF_POOL_DEFINE(zoap_data_pool, 4, ZOAP_BUF_SIZE, 0, NULL);

static const char *const test_path[] = { "test", NULL };

static struct in6_addr mcast_addr = MCAST_IP_ADDR;

struct dtls_timing_context {
	uint32_t snapshot;
	uint32_t int_ms;
	uint32_t fin_ms;
};

static void msg_dump(const char *s, uint8_t *data, unsigned int len)
{
	unsigned int i;

	printk("%s: ", s);
	for (i = 0; i < len; i++) {
		printk("%02x ", data[i]);
	}

	printk("(%u bytes)\n", len);
}

static int resource_reply_cb(const struct zoap_packet *response,
			     struct zoap_reply *reply,
			     const struct sockaddr *from)
{

	struct net_buf *frag = response->buf->frags;

	while (frag) {
		msg_dump("reply", frag->data, frag->len);
		frag = frag->frags;
	}

	return 0;
}

static void my_debug(void *ctx, int level,
		     const char *file, int line, const char *str)
{
	const char *p, *basename;

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}
	}

	mbedtls_printf("%s:%04d: |%d| %s", basename, line, level, str);
}

void dtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
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
	uint32_t seed;
	char *ptr = data;

	seed = sys_rand32_get();

	if (!seed) {
		seed = 7;
	}

	for (int i = 0; i < len; i++) {
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		*ptr++ = (char)seed;
	}

	*olen = len;
	return 0;
}

void dtls_client(void)
{
	int ret;
	struct udp_context ctx;
	struct dtls_timing_context timer;
	struct zoap_packet request, pkt;
	struct zoap_reply *reply;
	struct net_buf *nbuf, *frag;
	uint8_t observe = 0;
	const char *const *p;
	uint16_t len;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;

	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_platform_set_printf(printk);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_entropy_init(&entropy);
	mbedtls_entropy_add_source(&entropy, entropy_source, NULL,
				   MBEDTLS_ENTROPY_MAX_GATHER,
				   MBEDTLS_ENTROPY_SOURCE_STRONG);

	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				    (const unsigned char *)pers, strlen(pers));
	if (ret != 0) {
		mbedtls_printf("mbedtls_ctr_drbg_seed failed returned -0x%x\n",
			       -ret);
		goto exit;
	}

	ret = mbedtls_ssl_config_defaults(&conf,
					  MBEDTLS_SSL_IS_CLIENT,
					  MBEDTLS_SSL_TRANSPORT_DATAGRAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		mbedtls_printf("mbedtls_ssl_config_defaults"
			       " failed! returned -0x%x\n", -ret);
		goto exit;
	}

/* Modify this to change the default timeouts for the DTLS handshake */
/*        mbedtls_ssl_conf_handshake_timeout( &conf, min, max ); */

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(DEBUG_THRESHOLD);
#endif

	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_dbg(&conf, my_debug, NULL);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
	mbedtls_memory_buffer_alloc_init(heap, sizeof(heap));
#endif

	ret = mbedtls_ssl_setup(&ssl, &conf);

	if (ret != 0) {
		mbedtls_printf("mbedtls_ssl_setup failed returned -0x%x\n",
			       -ret);
		goto exit;
	}

	ret = udp_init(&ctx);
	if (ret != 0) {
		mbedtls_printf("udp_init failed returned 0x%x\n", ret);
		goto exit;
	}

	udp_tx(&ctx, payload, 32);

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	ret = mbedtls_ssl_conf_psk(&conf, psk, strlen(psk), psk_id,
				   strlen(psk_id));
	if (ret != 0) {
		mbedtls_printf("  failed\n  mbedtls_ssl_conf_psk"
			       " returned -0x%x\n", -ret);
		goto exit;
	}
#endif

	mbedtls_ssl_set_timer_cb(&ssl, &timer, dtls_timing_set_delay,
				 dtls_timing_get_delay);

	mbedtls_ssl_set_bio(&ssl, &ctx, udp_tx, NULL, udp_rx);

	do {
		ret = mbedtls_ssl_handshake(&ssl);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		 ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret != 0) {
		mbedtls_printf("mbedtls_ssl_handshake failed returned -0x%x\n",
			       -ret);
		goto exit;
	}

	/* Write to server */
retry:
	nbuf = net_buf_alloc(&zoap_nbuf_pool, K_NO_WAIT);
	if (!nbuf) {
		goto exit;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		goto exit;
	}

	net_buf_frag_add(nbuf, frag);

	ret = zoap_packet_init(&request, nbuf);
	if (ret < 0) {
		goto exit;
	}

	zoap_header_set_version(&request, 1);
	zoap_header_set_type(&request, ZOAP_TYPE_CON);
	zoap_header_set_code(&request, ZOAP_METHOD_GET);
	zoap_header_set_id(&request, zoap_next_id());
	zoap_header_set_token(&request, zoap_next_token(), 0);

	/* Enable observing the resource. */
	ret = zoap_add_option(&request, ZOAP_OPTION_OBSERVE,
			      &observe, sizeof(observe));
	if (ret < 0) {
		mbedtls_printf("Unable add option to request.\n");
		goto exit;
	}

	for (p = test_path; p && *p; p++) {
		ret = zoap_add_option(&request, ZOAP_OPTION_URI_PATH,
				      *p, strlen(*p));
		if (ret < 0) {
			mbedtls_printf("Unable add option/path to request.\n");
			goto exit;
		}
	}

	reply = zoap_reply_next_unused(replies, NUM_REPLIES);
	if (!reply) {
		mbedtls_printf("No resources for waiting for replies.\n");
		goto exit;
	}

	zoap_reply_init(reply, &request);
	reply->reply = resource_reply_cb;
	len = frag->len;

	do {
		ret = mbedtls_ssl_write(&ssl, frag->data, len);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		 ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	net_buf_unref(nbuf);

	if (ret <= 0) {
		mbedtls_printf("mbedtls_ssl_write failed returned 0x%x\n",
			       -ret);
		goto exit;
	}

	nbuf = net_buf_alloc(&zoap_nbuf_pool, K_NO_WAIT);
	if (!nbuf) {
		mbedtls_printf("Could not get buffer from pool\n");
		goto exit;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		mbedtls_printf("Could not get frag from pool\n");
		goto exit;
	}

	net_buf_frag_add(nbuf, frag);
	len = ZOAP_BUF_SIZE - 1;
	memset(frag->data, 0, ZOAP_BUF_SIZE);

	do {
		ret = mbedtls_ssl_read(&ssl, frag->data, ZOAP_BUF_SIZE - 1);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		 ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret <= 0) {
		net_buf_unref(nbuf);

		switch (ret) {
		case MBEDTLS_ERR_SSL_TIMEOUT:
			mbedtls_printf(" timeout\n");
			goto retry;

		case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
			mbedtls_printf(" connection was closed"
				       " gracefully\n");
			goto exit;

		default:
			mbedtls_printf(" mbedtls_ssl_read"
				       " returned -0x%x\n", -ret);
			goto exit;
		}
	}

	len = ret;
	frag->len = len;

	ret = zoap_packet_parse(&pkt, nbuf);
	if (ret) {
		mbedtls_printf("Could not parse packet\n");
		goto exit;
	}

	reply = zoap_response_received(&pkt, NULL, replies, NUM_REPLIES);
	if (!reply) {
		mbedtls_printf("No handler for response (%d)\n", ret);
	}

	net_buf_unref(nbuf);
	mbedtls_ssl_close_notify(&ssl);
exit:

	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
}

#define STACK_SIZE		4096
uint8_t stack[STACK_SIZE];

static inline int init_app(void)
{
#if defined(CONFIG_NET_SAMPLES_MY_IPV6_ADDR)
	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_SAMPLES_MY_IPV6_ADDR,
			  (struct sockaddr *)&client_addr) < 0) {
		mbedtls_printf("Invalid IPv6 address %s",
			       CONFIG_NET_SAMPLES_MY_IPV6_ADDR);
	}
#endif
	if (!net_if_ipv6_addr_add(net_if_get_default(), &client_addr,
				  NET_ADDR_MANUAL, 0)) {
		return -EIO;
	}

	net_if_ipv6_maddr_add(net_if_get_default(), &mcast_addr);

	return 0;
}

void main(void)
{
	if (init_app() != 0) {
		printk("Cannot initialize network\n");
		return;
	}

	k_thread_spawn(stack, STACK_SIZE, (k_thread_entry_t) dtls_client,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
