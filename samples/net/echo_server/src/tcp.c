/* tcp.c - TCP specific code for echo server */

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

#include <net/net_app.h>

#include "common.h"

static struct net_app_ctx tcp;

/* Note that both tcp and udp can share the same pool but in this
 * example the UDP context and TCP context have separate pools.
 */
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(echo_tx_tcp, 15);
NET_PKT_DATA_POOL_DEFINE(echo_data_tcp, 30);

static struct k_mem_slab *tx_tcp_slab(void)
{
	return &echo_tx_tcp;
}

static struct net_buf_pool *data_tcp_pool(void)
{
	return &echo_data_tcp;
}
#else
#define tx_tcp_slab NULL
#define data_tcp_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if defined(CONFIG_NET_APP_TLS)

/* The result buf size is set to large enough so that we can receive max size
 * buf back. Note that mbedtls needs also be configured to have equal size
 * value for its buffer size. See MBEDTLS_SSL_MAX_CONTENT_LEN option in TLS
 * config file.
 */
#define RESULT_BUF_SIZE 1500
static u8_t tls_result[RESULT_BUF_SIZE];

#if !defined(CONFIG_NET_APP_TLS_STACK_SIZE)
#define CONFIG_NET_APP_TLS_STACK_SIZE		8192
#endif /* CONFIG_NET_APP_TLS_STACK_SIZE */

#define APP_BANNER "Run TLS echo-server"
#define INSTANCE_INFO "Zephyr TLS echo-server #1"

/* Note that each net_app context needs its own stack as there will be
 * a separate thread needed.
 */
NET_STACK_DEFINE(NET_APP_TLS, net_app_tls_stack,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

#define RX_FIFO_DEPTH 4
K_MEM_POOL_DEFINE(ssl_pool, 4, 64, RX_FIFO_DEPTH, 4);
#endif /* CONFIG_NET_APP_TLS */

#if defined(CONFIG_NET_APP_TLS)
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
#endif /* CONFIG_NET_APP_TLS */

static void tcp_received(struct net_app_ctx *ctx,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	static char dbg[MAX_DBG_PRINT + 1];
	struct net_pkt *reply_pkt;
	sa_family_t family;
	int ret;

	if (!pkt) {
		/* EOF condition */
		return;
	}

	family = net_pkt_family(pkt);

	snprintk(dbg, MAX_DBG_PRINT, "TCP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	reply_pkt = build_reply_pkt(dbg, ctx, pkt, NET_TCPH_LEN);

	net_pkt_unref(pkt);

	if (!reply_pkt) {
		return;
	}

	ret = net_app_send_pkt(ctx, reply_pkt, NULL, 0, K_NO_WAIT,
			       UINT_TO_POINTER(net_pkt_get_len(reply_pkt)));
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		net_pkt_unref(reply_pkt);

		quit();
	}
}

void start_tcp(void)
{
	int ret;

	ret = net_app_init_tcp_server(&tcp, NULL, MY_PORT, NULL);
	if (ret < 0) {
		NET_ERR("Cannot init TCP service at port %d", MY_PORT);
		return;
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&tcp, tx_tcp_slab, data_tcp_pool);
#endif

	ret = net_app_set_cb(&tcp, NULL, tcp_received, NULL, NULL);
	if (ret < 0) {
		NET_ERR("Cannot set callbacks (%d)", ret);
		net_app_release(&tcp);
		return;
	}

#if defined(CONFIG_NET_APP_TLS)
	ret = net_app_server_tls(&tcp,
				 tls_result,
				 sizeof(tls_result),
				 APP_BANNER,
				 INSTANCE_INFO,
				 strlen(INSTANCE_INFO),
				 setup_cert,
				 NULL,
				 &ssl_pool,
				 net_app_tls_stack,
				 K_THREAD_STACK_SIZEOF(net_app_tls_stack));
	if (ret < 0) {
		NET_ERR("Cannot init TLS");
	}
#endif

	net_app_server_enable(&tcp);

	ret = net_app_listen(&tcp);
	if (ret < 0) {
		NET_ERR("Cannot wait connection (%d)", ret);
		net_app_release(&tcp);
		return;
	}
}

void stop_tcp(void)
{
	net_app_server_disable(&tcp);

	net_app_close(&tcp);
	net_app_release(&tcp);
}
