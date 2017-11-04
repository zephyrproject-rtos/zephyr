/*  CoAP over DTLS server implemented with mbedTLS.
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
#include <misc/byteorder.h>

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
#include <net/net_pkt.h>
#include <net/net_ip.h>

#include <net/coap.h>

#include "udp.h"
#include "udp_cfg.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ssl_cookie.h"

#if defined(MBEDTLS_DEBUG_C)
#include "mbedtls/debug.h"
#define DEBUG_THRESHOLD 0
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
static unsigned char heap[8192];
#endif

#define COAP_BUF_SIZE 128

NET_PKT_TX_SLAB_DEFINE(coap_pkt_slab, 4);
NET_BUF_POOL_DEFINE(coap_data_pool, 4, COAP_BUF_SIZE, 0, NULL);

/*
 * Hardcoded values for server host and port
 */

const char *pers = "dtsl_server";

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
const unsigned char psk[] = "passwd\0";
const char psk_id[] = "Client_identity\0";
#endif

static mbedtls_ssl_context *curr_ctx;

static int send_response(struct coap_packet *request, u8_t response_code)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct coap_packet response;
	u8_t code, type;
	u16_t id;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);

	printk("*******\n");
	printk("type: %u code %u id %u\n", type, code, id);
	printk("*******\n");

	pkt = net_pkt_get_reserve(&coap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_buf_alloc(&coap_data_pool, K_NO_WAIT);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     0, NULL, response_code, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	do {
		r = mbedtls_ssl_write(curr_ctx, frag->data, frag->len);
	} while (r == MBEDTLS_ERR_SSL_WANT_READ
		 || r == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (r >= 0) {
		r = 0;
	}

	net_pkt_unref(pkt);

	return r;
}

static int test_del(struct coap_resource *resource,
		    struct coap_packet *request)
{
	return send_response(request, COAP_RESPONSE_CODE_DELETED);
}

static int test_put(struct coap_resource *resource,
		    struct coap_packet *request)
{
	return send_response(request, COAP_RESPONSE_CODE_CHANGED);
}

static int test_post(struct coap_resource *resource,
		     struct coap_packet *request)
{
	return send_response(request, COAP_RESPONSE_CODE_CREATED);
}

static int piggyback_get(struct coap_resource *resource,
			 struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct coap_packet response;
	u8_t payload[40], code, type;
	u16_t id;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);

	printk("*******\n");
	printk("type: %u code %u id %u\n", type, code, id);
	printk("*******\n");

	pkt = net_pkt_get_reserve(&coap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_buf_alloc(&coap_data_pool, K_NO_WAIT);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     0, NULL, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *)payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	do {
		r = mbedtls_ssl_write(curr_ctx, frag->data, frag->len);
	} while (r == MBEDTLS_ERR_SSL_WANT_READ
		 || r == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (r >= 0) {
		r = 0;
	}

	net_pkt_unref(pkt);

	return r;
}

static int query_get(struct coap_resource *resource,
		     struct coap_packet *request)
{
	struct coap_option options[4];
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct coap_packet response;
	u8_t payload[40], code, type;
	u16_t id;
	int i, r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);

	r = coap_find_options(request, COAP_OPTION_URI_QUERY, options, 4);
	if (r <= 0) {
		return -EINVAL;
	}

	printk("*******\n");
	printk("type: %u code %u id %u\n", type, code, id);
	printk("num queries: %d\n", r);

	for (i = 0; i < r; i++) {
		char str[16];

		if (options[i].len + 1 > sizeof(str)) {
			printk("Unexpected length of query: "
			       "%d (expected %zu)\n",
			       options[i].len, sizeof(str));
			break;
		}

		memcpy(str, options[i].value, options[i].len);
		str[options[i].len] = '\0';

		printk("query[%d]: %s\n", i + 1, str);
	}

	printk("*******\n");

	pkt = net_pkt_get_reserve(&coap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_buf_alloc(&coap_data_pool, K_NO_WAIT);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     0, NULL, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *)payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	do {
		r = mbedtls_ssl_write(curr_ctx, frag->data, frag->len);
	} while (r == MBEDTLS_ERR_SSL_WANT_READ
		 || r == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (r >= 0) {
		r = 0;
	}

	net_pkt_unref(pkt);

	return r;
}

static const char *const test_path[] = { "test", NULL };

static const char *const segments_path[] = { "seg1", "seg2", "seg3", NULL };

static const char *const query_path[] = { "query", NULL };

static struct coap_resource resources[] = {
	{.get = piggyback_get,
	 .post = test_post,
	 .del = test_del,
	 .put = test_put,
	 .path = test_path},
	{.get = piggyback_get,
	 .path = segments_path,
	 },
	{.get = query_get,
	 .path = query_path,
	 },
	{},
};

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

void dtls_server(void)
{
	int len, ret = 0;
	struct udp_context ctx;
	struct dtls_timing_context timer;
	struct coap_packet zpkt;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct coap_option options[16];
	u8_t opt_num = 16;

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

	mbedtls_entropy_init(&entropy);
	mbedtls_entropy_add_source(&entropy, entropy_source, NULL,
				   MBEDTLS_ENTROPY_MAX_GATHER,
				   MBEDTLS_ENTROPY_SOURCE_STRONG);

	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				    (const unsigned char *)pers, strlen(pers));
	if (ret != 0) {
		mbedtls_printf(" failed!\n"
			       " mbedtls_ctr_drbg_seed returned -0x%x\n", -ret);
		goto exit;
	}

	ret = mbedtls_ssl_config_defaults(&conf,
					  MBEDTLS_SSL_IS_SERVER,
					  MBEDTLS_SSL_TRANSPORT_DATAGRAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		mbedtls_printf(" failed!\n"
			       " mbedtls_ssl_config_defaults returned -0x%x\n",
			       -ret);
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
		mbedtls_printf(" failed!\n"
			       " mbedtls_ssl_cookie_setup returned -0x%x\n",
			       -ret);
		goto exit;
	}

	mbedtls_ssl_conf_dtls_cookies(&conf, mbedtls_ssl_cookie_write,
				      mbedtls_ssl_cookie_check, &cookie_ctx);

	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0) {
		mbedtls_printf(" failed!\n"
			       " mbedtls_ssl_setup returned -0x%x\n", -ret);
		goto exit;
	}

	ret = udp_init(&ctx);
	if (ret != 0) {
		mbedtls_printf(" failed!\n udp_init returned 0x%x\n", ret);
		goto exit;
	}

reset:
	mbedtls_ssl_session_reset(&ssl);

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	ret = mbedtls_ssl_conf_psk(&conf, psk, strlen((char *)psk),
				   (unsigned char *)psk_id,
				   strlen(psk_id));
	if (ret != 0) {
		mbedtls_printf("  failed!\n  mbedtls_ssl_conf_psk"
			       " returned -0x%04X\n", -ret);
		goto exit;
	}
#endif

	mbedtls_ssl_set_timer_cb(&ssl, &timer, dtls_timing_set_delay,
				 dtls_timing_get_delay);

	mbedtls_ssl_set_bio(&ssl, &ctx, udp_tx, udp_rx, NULL);

	/* For HelloVerifyRequest cookies */
	ctx.client_id = (char)ctx.remaining;

	ret = mbedtls_ssl_set_client_transport_id(
		&ssl, (unsigned char *)&ctx.client_id, sizeof(char));
	if (ret != 0) {
		mbedtls_printf(" failed!\n"
			       " mbedtls_ssl_set_client_transport_id()"
			       " returned -0x%x\n", -ret);
		goto exit;
	}

	curr_ctx = &ssl;

	do {
		ret = mbedtls_ssl_handshake(&ssl);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		 ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
		ret = 0;
		goto reset;
	}

	if (ret != 0) {
		mbedtls_printf(" failed!\n"
			       "  mbedtls_ssl_handshake returned -0x%x\n",
			       -ret);
		goto reset;
	}

	do {
		struct net_buf *ip;

		/* Read the request */
		pkt = net_pkt_get_reserve(&coap_pkt_slab, 0, K_NO_WAIT);
		if (!pkt) {
			mbedtls_printf("Could not get packet from slab\n");
			goto exit;
		}

		frag = net_buf_alloc(&coap_data_pool, K_NO_WAIT);
		if (!frag) {
			mbedtls_printf("Could not get frag from pool\n");
			goto exit;
		}

		net_pkt_frag_add(pkt, frag);
		len = COAP_BUF_SIZE - 1;
		memset(frag->data, 0, COAP_BUF_SIZE);

		ret = mbedtls_ssl_read(&ssl, frag->data, len);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}

		if (ret <= 0) {
			net_pkt_unref(pkt);

			switch (ret) {
			case MBEDTLS_ERR_SSL_TIMEOUT:
				mbedtls_printf(" timeout\n");
				goto reset;

			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				mbedtls_printf(" connection was closed"
					       " gracefully\n");
				goto close_notify;

			default:
				mbedtls_printf(" mbedtls_ssl_read"
					       " returned -0x%x\n", -ret);
				goto reset;
			}
		}

		len = ret;
		frag->len = len;

		/* The COAP packet does not have IP + UDP header and the coap
		 * packet starts immediately from first fragment. The net_pkt
		 * contains information from which byte in net_buf the coap
		 * packet actually starts and coap API will use that info when
		 * parsing the packet. Because of this add a dummy IP + UDP
		 * header before the actual coap data so that parsing succeeds.
		 */
		ip = net_buf_alloc(&coap_data_pool, K_NO_WAIT);
		if (!ip) {
			mbedtls_printf("Could not get frag from pool\n");
			goto exit;
		}

		net_buf_add(ip, net_pkt_ip_hdr_len(pkt) +
			    net_pkt_ipv6_ext_len(pkt) + NET_UDPH_LEN);
		ip->frags = pkt->frags;
		pkt->frags = ip;

		ret = coap_packet_parse(&zpkt, pkt, options, opt_num);
		if (ret) {
			mbedtls_printf("Could not parse packet\n");
			goto exit;
		}

		ret = coap_handle_request(&zpkt, resources, options, opt_num);
		if (ret < 0) {
			mbedtls_printf("No handler for such request (%d)\n",
				       ret);
		}

		net_pkt_unref(pkt);

	} while (1);

close_notify:
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

#define STACK_SIZE		4096
K_THREAD_STACK_DEFINE(stack, STACK_SIZE);
static struct k_thread thread_data;

static inline int init_app(void)
{
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

	return 0;
}

void main(void)
{
	if (init_app() != 0) {
		printk("Cannot initialize network\n");
		return;
	}

	k_thread_create(&thread_data, stack, STACK_SIZE,
			(k_thread_entry_t) dtls_server,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);

}
