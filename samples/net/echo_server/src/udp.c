/* udp.c - UDP specific code for echo server */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "echo-server"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/udp.h>

#include <net/net_app.h>

#include "common.h"

static struct net_app_ctx udp;

/* Note that both tcp and udp can share the same pool but in this
 * example the UDP context and TCP context have separate pools.
 */
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(echo_tx_udp, 5);
NET_PKT_DATA_POOL_DEFINE(echo_data_udp, 20);

static struct k_mem_slab *tx_udp_slab(void)
{
	return &echo_tx_udp;
}

static struct net_buf_pool *data_udp_pool(void)
{
	return &echo_data_udp;
}
#else
#define tx_udp_slab NULL
#define data_udp_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if defined(CONFIG_NET_APP_DTLS)

/* The result buf size is set to large enough so that we can receive max size
 * buf back. Note that mbedtls needs also be configured to have equal size
 * value for its buffer size. See MBEDTLS_SSL_MAX_CONTENT_LEN option in TLS
 * config file.
 */
#define RESULT_BUF_SIZE 1500
static u8_t dtls_result[RESULT_BUF_SIZE];

#define APP_BANNER "Run DTLS echo-server"
#define INSTANCE_INFO "Zephyr DTLS echo-server #1"

/* Note that each net_app context needs its own stack as there will be
 * a separate thread needed.
 */
NET_STACK_DEFINE(NET_APP_DTLS, net_app_dtls_stack,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

#define RX_FIFO_DEPTH 4
K_MEM_POOL_DEFINE(dtls_pool, 4, 64, RX_FIFO_DEPTH, 4);
#endif /* CONFIG_NET_APP_TLS */

#if defined(CONFIG_NET_APP_DTLS)
/* Load the certificates and private RSA key. */

#include "test_certs.h"

static int setup_cert(struct net_app_ctx *ctx,
		      mbedtls_x509_crt *cert,
		      mbedtls_pk_context *pkey)
{
	int ret;

	ret = mbedtls_x509_crt_parse(cert, rsa_example_cert_der,
				     rsa_example_cert_der_len);
	if (ret != 0) {
		NET_ERR("mbedtls_x509_crt_parse returned %d", ret);
		return ret;
	}

	ret = mbedtls_pk_parse_key(pkey, rsa_example_keypair_der,
				   rsa_example_keypair_der_len, NULL, 0);
	if (ret != 0) {
		NET_ERR("mbedtls_pk_parse_key returned %d", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_NET_APP_DTLS */

static inline void set_dst_addr(sa_family_t family,
				struct net_pkt *pkt,
				struct sockaddr *dst_addr)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		return;
	}

#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr,
				&NET_IPV6_HDR(pkt)->src);
		net_sin6(dst_addr)->sin6_family = AF_INET6;
		net_sin6(dst_addr)->sin6_port = udp_hdr->src_port;
	}
#endif /* CONFIG_NET_IPV6) */

#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		net_ipaddr_copy(&net_sin(dst_addr)->sin_addr,
				&NET_IPV4_HDR(pkt)->src);
		net_sin(dst_addr)->sin_family = AF_INET;
		net_sin(dst_addr)->sin_port = udp_hdr->src_port;
	}
#endif /* CONFIG_NET_IPV6) */
}

static void udp_received(struct net_app_ctx *ctx,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	static char dbg[MAX_DBG_PRINT + 1];
	struct net_pkt *reply_pkt;
	struct sockaddr dst_addr;
	sa_family_t family = net_pkt_family(pkt);
	socklen_t dst_len;
	u32_t pkt_len;
	int ret;

	snprintk(dbg, MAX_DBG_PRINT, "UDP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	if (family == AF_INET6) {
		dst_len = sizeof(struct sockaddr_in6);
	} else {
		dst_len = sizeof(struct sockaddr_in);
	}

	/* Note that for DTLS swapping the source/destination address has no
	 * effect as the user data is sent in a DTLS tunnel where tunnel end
	 * points are already set.
	 */
	set_dst_addr(family, pkt, &dst_addr);

	reply_pkt = build_reply_pkt(dbg, ctx, pkt);

	net_pkt_unref(pkt);

	if (!reply_pkt) {
		return;
	}

	pkt_len = net_pkt_appdatalen(reply_pkt);

	ret = net_app_send_pkt(ctx, reply_pkt, &dst_addr, dst_len, K_NO_WAIT,
			       UINT_TO_POINTER(pkt_len));
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		net_pkt_unref(reply_pkt);
	}
}

void start_udp(void)
{
	int ret;

	ret = net_app_init_udp_server(&udp, NULL, MY_PORT, NULL);
	if (ret < 0) {
		NET_ERR("Cannot init UDP service at port %d", MY_PORT);
		return;
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&udp, tx_udp_slab, data_udp_pool);
#endif

	ret = net_app_set_cb(&udp, NULL, udp_received, pkt_sent, NULL);
	if (ret < 0) {
		NET_ERR("Cannot set callbacks (%d)", ret);
		net_app_release(&udp);
		return;
	}

#if defined(CONFIG_NET_APP_DTLS)
	ret = net_app_server_tls(&udp,
				 dtls_result,
				 sizeof(dtls_result),
				 APP_BANNER,
				 INSTANCE_INFO,
				 strlen(INSTANCE_INFO),
				 setup_cert,
				 NULL,
				 &dtls_pool,
				 net_app_dtls_stack,
				 K_THREAD_STACK_SIZEOF(net_app_dtls_stack));
	if (ret < 0) {
		NET_ERR("Cannot init DTLS");
	}
#endif

	net_app_server_enable(&udp);

	ret = net_app_listen(&udp);
	if (ret < 0) {
		NET_ERR("Cannot wait connection (%d)", ret);
		net_app_release(&udp);
		return;
	}
}

void stop_udp(void)
{
	net_app_close(&udp);
	net_app_release(&udp);
}
