/* tcp.c - TCP specific code for echo client */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "echo-client"
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

static struct net_app_ctx tcp6;
static struct net_app_ctx tcp4;

static int connected_count;

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
#define HOSTNAME "localhost"   /* for cert verification if that is enabled */

/* The result buf size is set to large enough so that we can receive max size
 * buf back. Note that mbedtls needs also be configured to have equal size
 * value for its buffer size. See MBEDTLS_SSL_MAX_CONTENT_LEN option in TLS
 * config file.
 */
#define RESULT_BUF_SIZE 1500
static u8_t tls_result_ipv6[RESULT_BUF_SIZE];
static u8_t tls_result_ipv4[RESULT_BUF_SIZE];

#if !defined(CONFIG_NET_APP_TLS_STACK_SIZE)
#define CONFIG_NET_APP_TLS_STACK_SIZE		6144
#endif /* CONFIG_NET_APP_TLS_STACK_SIZE */

#define INSTANCE_INFO "Zephyr TLS echo-client #1"

/* Note that each net_app context needs its own stack as there will be
 * a separate thread needed.
 */
NET_STACK_DEFINE(NET_APP_TLS_IPv4, net_app_tls_stack_ipv4,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

NET_STACK_DEFINE(NET_APP_TLS_IPv6, net_app_tls_stack_ipv6,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

NET_APP_TLS_POOL_DEFINE(ssl_pool, 10);
#else
#define tls_result_ipv6 NULL
#define tls_result_ipv4 NULL
#define net_app_tls_stack_ipv4 NULL
#define net_app_tls_stack_ipv6 NULL
#endif /* CONFIG_NET_APP_TLS */

#if defined(CONFIG_NET_APP_TLS)
/* Load the certificates and private RSA key. */

#include "test_certs.h"

static int setup_cert(struct net_app_ctx *ctx, void *cert)
{
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	mbedtls_ssl_conf_psk(&ctx->tls.mbedtls.conf,
			     client_psk, sizeof(client_psk),
			     (const unsigned char *)client_psk_id,
			     sizeof(client_psk_id) - 1);
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	{
		mbedtls_x509_crt *ca_cert = cert;
		int ret;

		ret = mbedtls_x509_crt_parse_der(ca_cert,
						 ca_certificate,
						 sizeof(ca_certificate));
		if (ret != 0) {
			NET_ERR("mbedtls_x509_crt_parse_der failed "
				"(-0x%x)", -ret);
			return ret;
		}

		mbedtls_ssl_conf_ca_chain(&ctx->tls.mbedtls.conf,
					  ca_cert, NULL);

		/* In this example, we skip the certificate checks. In real
		 * life scenarios, one should always verify the certificates.
		 */
		mbedtls_ssl_conf_authmode(&ctx->tls.mbedtls.conf,
					  MBEDTLS_SSL_VERIFY_REQUIRED);

		mbedtls_ssl_conf_cert_profile(&ctx->tls.mbedtls.conf,
					    &mbedtls_x509_crt_profile_default);
#define VERIFY_CERTS 0
#if VERIFY_CERTS
		mbedtls_ssl_conf_authmode(&ctx->tls.mbedtls.conf,
					  MBEDTLS_SSL_VERIFY_OPTIONAL);
#else
		;
#endif /* VERIFY_CERTS */
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return 0;
}
#endif /* CONFIG_NET_APP_TLS */

static void send_tcp_data(struct net_app_ctx *ctx,
			  struct data *data)
{
	struct net_pkt *pkt;
	size_t len;
	int ret;

	do {
		data->expecting_tcp = sys_rand32_get() % ipsum_len;
	} while (data->expecting_tcp == 0);

	data->received_tcp = 0;

	pkt = prepare_send_pkt(ctx, data->proto, &data->expecting_tcp);
	if (!pkt) {
		return;
	}

	len = net_pkt_get_len(pkt);

	NET_ASSERT_INFO(data->expecting_tcp == len,
			"%s data to send %d bytes, real len %zu",
			data->proto, data->expecting_tcp, len);

	ret = net_app_send_pkt(ctx, pkt, NULL, 0, K_FOREVER,
			       UINT_TO_POINTER(len));
	if (ret < 0) {
		NET_ERR("Cannot send %s data to peer (%d)", data->proto, ret);
		net_pkt_unref(pkt);
	}
}

static bool compare_tcp_data(struct net_pkt *pkt, int expecting_len,
			     int received_len)
{
	u8_t *ptr = net_pkt_appdata(pkt);
	const char *start;
	int pos = 0;
	struct net_buf *frag;
	int len;

	/* frag will point to first fragment with IP header in it.
	 */
	frag = pkt->frags;

	/* Do not include the protocol headers for the first fragment.
	 * The remaining fragments contain only data so the user data
	 * length is directly the fragment len.
	 */
	len = frag->len - (ptr - frag->data);

	start = lorem_ipsum + received_len;

	while (frag) {
		if (memcmp(ptr, start + pos, len)) {
			NET_DBG("Invalid data received");
			return false;
		}

		pos += len;

		frag = frag->frags;
		if (!frag) {
			break;
		}

		ptr = frag->data;
		len = frag->len;
	}

	NET_DBG("Compared %d bytes, all ok", net_pkt_appdatalen(pkt));

	return true;
}

static void tcp_received(struct net_app_ctx *ctx,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	struct data *data = ctx->user_data;

	ARG_UNUSED(user_data);
	ARG_UNUSED(status);

	if (!pkt || net_pkt_appdatalen(pkt) == 0) {
		if (pkt) {
			net_pkt_unref(pkt);
		}

		return;
	}

	NET_DBG("%s: Sent %d bytes, received %u bytes",
		data->proto, data->expecting_tcp, net_pkt_appdatalen(pkt));

	if (!compare_tcp_data(pkt, data->expecting_tcp, data->received_tcp)) {
		NET_DBG("Data mismatch");
	} else {
		data->received_tcp += net_pkt_appdatalen(pkt);
	}

	if (data->expecting_tcp <= data->received_tcp) {
		/* Send more data */
		send_tcp_data(ctx, data);
	}

	net_pkt_unref(pkt);
}

static void tcp_connected(struct net_app_ctx *ctx,
			  int status,
			  void *user_data)
{
	if (status < 0) {
		return;
	}

	connected_count++;

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    IS_ENABLED(CONFIG_NET_IPV4)) {
			if (connected_count > 1) {
				k_sem_give(&tcp_ready);
			}
		} else {
			k_sem_give(&tcp_ready);
		}
	}
}

static int connect_tcp(struct net_app_ctx *ctx, const char *peer,
		       void *user_data, u8_t *result_buf,
		       size_t result_buf_len,
		       k_thread_stack_t *stack, size_t stack_size)
{
	struct data *data = user_data;
	int ret;

	ret = net_app_init_tcp_client(ctx, NULL, NULL, peer, PEER_PORT,
				      WAIT_TIME, user_data);
	if (ret < 0) {
		NET_ERR("Cannot init %s TCP client (%d)", data->proto, ret);
		goto fail;
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(ctx, tx_tcp_slab, data_tcp_pool);
#endif

	ret = net_app_set_cb(ctx, tcp_connected, tcp_received, NULL, NULL);
	if (ret < 0) {
		NET_ERR("Cannot set callbacks (%d)", ret);
		goto fail;
	}

#if defined(CONFIG_NET_APP_TLS)
	ret = net_app_client_tls(ctx,
				 result_buf,
				 result_buf_len,
				 INSTANCE_INFO,
				 strlen(INSTANCE_INFO),
				 setup_cert,
				 HOSTNAME,
				 NULL,
				 &ssl_pool,
				 stack,
				 stack_size);
	if (ret < 0) {
		NET_ERR("Cannot init TLS");
		goto fail;
	}
#endif

	ret = net_app_connect(ctx, CONNECT_TIME);
	if (ret < 0) {
		NET_ERR("Cannot connect TCP (%d)", ret);
		goto fail;
	}

fail:
	return ret;
}

int start_tcp(void)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = connect_tcp(&tcp6, CONFIG_NET_APP_PEER_IPV6_ADDR,
				  &conf.ipv6, tls_result_ipv6,
				  sizeof(tls_result_ipv6),
				  net_app_tls_stack_ipv6,
				  K_THREAD_STACK_SIZEOF(
					  net_app_tls_stack_ipv6));
		if (ret < 0) {
			NET_ERR("Cannot init IPv6 TCP client (%d)", ret);
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = connect_tcp(&tcp4, CONFIG_NET_APP_PEER_IPV4_ADDR,
				  &conf.ipv4, tls_result_ipv4,
				  sizeof(tls_result_ipv4),
				  net_app_tls_stack_ipv4,
				  K_THREAD_STACK_SIZEOF(
					  net_app_tls_stack_ipv4));
		if (ret < 0) {
			NET_ERR("Cannot init IPv4 TCP client (%d)", ret);
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		send_tcp_data(&tcp6, &conf.ipv6);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		send_tcp_data(&tcp4, &conf.ipv4);
	}

	return ret;
}

void stop_tcp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_app_close(&tcp6);
		net_app_release(&tcp6);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_app_close(&tcp4);
		net_app_release(&tcp4);
	}
}
