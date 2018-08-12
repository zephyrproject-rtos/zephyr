/* udp.c - UDP specific code for echo client */

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

static struct net_app_ctx udp6;
static struct net_app_ctx udp4;

#define UDP_SLEEP K_MSEC(150)

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
#define HOSTNAME "localhost"   /* for cert verification if that is enabled */

/* The result buf size is set to large enough so that we can receive max size
 * buf back. Note that mbedtls needs also be configured to have equal size
 * value for its buffer size. See MBEDTLS_SSL_MAX_CONTENT_LEN option in DTLS
 * config file.
 */
#define RESULT_BUF_SIZE 1500
static u8_t dtls_result_ipv6[RESULT_BUF_SIZE];
static u8_t dtls_result_ipv4[RESULT_BUF_SIZE];

#if !defined(CONFIG_NET_APP_TLS_STACK_SIZE)
#define CONFIG_NET_APP_TLS_STACK_SIZE		6144
#endif /* CONFIG_NET_APP_TLS_STACK_SIZE */

#define INSTANCE_INFO "Zephyr DTLS echo-client #1"

/* Note that each net_app context needs its own stack as there will be
 * a separate thread needed.
 */
NET_STACK_DEFINE(NET_APP_DTLS_IPv4, net_app_dtls_stack_ipv4,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

NET_STACK_DEFINE(NET_APP_DTLS_IPv6, net_app_dtls_stack_ipv6,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

NET_APP_TLS_POOL_DEFINE(dtls_pool, 10);
#else
#define dtls_result_ipv6 NULL
#define dtls_result_ipv4 NULL
#define net_app_dtls_stack_ipv4 NULL
#define net_app_dtls_stack_ipv6 NULL
#endif /* CONFIG_NET_APP_TLS */

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
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

static void send_udp_data(struct net_app_ctx *ctx, struct data *data)
{
	struct net_pkt *pkt;
	size_t len;
	int ret;

	data->expecting_udp = sys_rand32_get() % ipsum_len;

	pkt = prepare_send_pkt(ctx, data->proto, &data->expecting_udp);
	if (!pkt) {
		return;
	}

	len = net_pkt_get_len(pkt);

	NET_ASSERT_INFO(data->expecting_udp == len,
			"Data to send %d bytes, real len %zu",
			data->expecting_udp, len);

	ret = net_app_send_pkt(ctx, pkt, NULL, 0, K_FOREVER,
			       UINT_TO_POINTER(len));
	if (ret < 0) {
		NET_ERR("Cannot send %s data to peer (%d)", data->proto, ret);

		net_pkt_unref(pkt);
	}

	k_delayed_work_submit(&data->recv, WAIT_TIME);
}

static bool compare_udp_data(struct net_pkt *pkt, int expecting_len)
{
	u8_t *ptr = net_pkt_appdata(pkt);
	struct net_buf *frag;
	int pos = 0;
	int len;

	/* frag will now point to first fragment with IP header
	 * in it.
	 */
	frag = pkt->frags;

	/* Do not include the protocol headers in the first fragment.
	 * The remaining fragments contain only data so the user data
	 * length is directly the fragment len.
	 */
	len = frag->len - (ptr - frag->data);

	while (frag) {
		if (memcmp(ptr, lorem_ipsum + pos, len)) {
			NET_DBG("Invalid data received");
			return false;
		} else {
			pos += len;

			frag = frag->frags;
			if (!frag) {
				break;
			}

			ptr = frag->data;
			len = frag->len;
		}
	}

	NET_DBG("Compared %d bytes, all ok", expecting_len);

	return true;
}

static void udp_received(struct net_app_ctx *ctx,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	struct data *data = ctx->user_data;

	ARG_UNUSED(user_data);
	ARG_UNUSED(status);

	if (data->expecting_udp != net_pkt_appdatalen(pkt)) {
		NET_DBG("Sent %d bytes, received %u bytes",
			data->expecting_udp, net_pkt_appdatalen(pkt));
	}

	if (!compare_udp_data(pkt, data->expecting_udp)) {
		NET_DBG("Data mismatch");
	}

	net_pkt_unref(pkt);

	k_delayed_work_cancel(&data->recv);

	/* Do not flood the link if we have also TCP configured */
	if (IS_ENABLED(CONFIG_NET_TCP)) {
		k_sleep(UDP_SLEEP);
	}

	send_udp_data(ctx, data);
}

/* We can start to send data when UDP is "connected" */
static void udp_connected(struct net_app_ctx *ctx,
			  int status,
			  void *user_data)
{
	struct data *data = user_data;

	data->udp = ctx;

	send_udp_data(ctx, data);
}

static int connect_udp(struct net_app_ctx *ctx, const char *peer,
		       void *user_data, u8_t *dtls_result_buf,
		       size_t dtls_result_buf_len,
		       k_thread_stack_t *stack, size_t stack_size)
{
	struct data *data = user_data;
	int ret;

	ret = net_app_init_udp_client(ctx, NULL, NULL, peer, PEER_PORT,
				      WAIT_TIME, user_data);
	if (ret < 0) {
		NET_ERR("Cannot init %s UDP client (%d)", data->proto, ret);
		goto fail;
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(ctx, tx_udp_slab, data_udp_pool);
#endif

	ret = net_app_set_cb(ctx, udp_connected, udp_received, NULL, NULL);
	if (ret < 0) {
		NET_ERR("Cannot set callbacks (%d)", ret);
		goto fail;
	}

#if defined(CONFIG_NET_APP_DTLS)
	ret = net_app_client_tls(ctx,
				 dtls_result_buf,
				 dtls_result_buf_len,
				 INSTANCE_INFO,
				 strlen(INSTANCE_INFO),
				 setup_cert,
				 HOSTNAME,
				 NULL,
				 &dtls_pool,
				 stack,
				 stack_size);
	if (ret < 0) {
		NET_ERR("Cannot init DTLS");
		goto fail;
	}
#else
	ARG_UNUSED(dtls_result_buf);
	ARG_UNUSED(dtls_result_buf_len);
	ARG_UNUSED(stack);
	ARG_UNUSED(stack_size);
#endif

	ret = net_app_connect(ctx, CONNECT_TIME);
	if (ret < 0) {
		NET_ERR("Cannot connect UDP (%d)", ret);
		goto fail;
	}

fail:
	return ret;
}

static void wait_reply(struct k_work *work)
{
	/* This means that we did not receive response in time. */
	struct data *data = CONTAINER_OF(work, struct data, recv);

	NET_ERR("Data packet not received");

	/* Send a new packet at this point */
	send_udp_data(data->udp, data);
}

void start_udp(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_delayed_work_init(&conf.ipv6.recv, wait_reply);

		ret = connect_udp(&udp6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
				  &conf.ipv6, dtls_result_ipv6,
				  sizeof(dtls_result_ipv6),
				  net_app_dtls_stack_ipv6,
				  K_THREAD_STACK_SIZEOF(
					  net_app_dtls_stack_ipv6));
		if (ret < 0) {
			NET_ERR("Cannot init IPv6 UDP client (%d)", ret);
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_delayed_work_init(&conf.ipv4.recv, wait_reply);

		ret = connect_udp(&udp4, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
				  &conf.ipv4, dtls_result_ipv4,
				  sizeof(dtls_result_ipv4),
				  net_app_dtls_stack_ipv4,
				  K_THREAD_STACK_SIZEOF(
					  net_app_dtls_stack_ipv4));
		if (ret < 0) {
			NET_ERR("Cannot init IPv4 UDP client (%d)", ret);
		}
	}

}

void stop_udp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_app_close(&udp6);
		net_app_release(&udp6);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_app_close(&udp4);
		net_app_release(&udp4);
	}
}
