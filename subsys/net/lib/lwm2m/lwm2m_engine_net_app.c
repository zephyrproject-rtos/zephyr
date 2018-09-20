/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lib/lwm2m_engine_net_app"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>

#include <zephyr/types.h>
#include <net/net_app.h>
#include <net/net_pkt.h>
#include <net/udp.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#include "lwm2m_engine_net_app.h"

/* TODO: Make Kconfig? */
#define MAX_IN_BUF	3

u8_t in_buf[MAX_IN_BUF][MAX_PACKET_SIZE];
bool in_buf_used[MAX_IN_BUF];

/* incoming message buf functions */

static u8_t *get_buf(void)
{
	int i;

	for (i = 0; i < MAX_IN_BUF; i++) {
		if (!in_buf_used[i]) {
			in_buf_used[i] = true;
			return in_buf[i];
		}
	}

	return NULL;
}

static int free_buf(u8_t *buf)
{
	int i;

	for (i = 0; i < MAX_IN_BUF; i++) {
		if (in_buf[i] == buf) {
			in_buf_used[i] = false;
			return 0;
		}
	}


	return -ENOENT;
}

int lwm2m_nl_net_app_msg_send(struct lwm2m_message *msg)
{
	struct net_layer_net_app *nl_data;
	struct net_pkt *pkt;
	struct net_buf *frag;
	int ret;

	if (!msg) {
		return -EINVAL;
	}

	nl_data = (struct net_layer_net_app *)
			lwm2m_nl_api_from_ctx(msg->ctx)->nl_user_data;
	pkt = net_app_get_net_pkt(&nl_data->net_app_ctx, AF_UNSPEC,
				  BUF_ALLOC_TIMEOUT);
	if (!pkt) {
		SYS_LOG_ERR("Unable to get TX packet.");
		return -ENOMEM;
	}

	frag = net_app_get_net_buf(&nl_data->net_app_ctx, pkt,
				   BUF_ALLOC_TIMEOUT);
	if (!frag) {
		SYS_LOG_ERR("Unable to get DATA buffer.");
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	if (!net_pkt_append_all(pkt, msg->cpkt.fbuf.buf_len,
				msg->cpkt.fbuf.buf, BUF_ALLOC_TIMEOUT)) {
		SYS_LOG_ERR("Unable to append packet data.");
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	ret = net_app_send_pkt(&nl_data->net_app_ctx, pkt,
			       &msg->ctx->remote_addr, NET_SOCKADDR_MAX_SIZE,
			       K_NO_WAIT, NULL);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

void lwm2m_engine_udp_receive(struct net_app_ctx *app_ctx, struct net_pkt *pkt,
			      int status, void *user_data)
{
	struct net_layer_net_app *nl_data;
	struct sockaddr from_addr;
	struct net_udp_hdr hdr, *udp_hdr;
	u8_t *buf = NULL;
	size_t hdr_len;
	int ret;

	nl_data = CONTAINER_OF(app_ctx, struct net_layer_net_app, net_app_ctx);
	if (!nl_data) {
		SYS_LOG_ERR("No networking layer!");
		goto cleanup;
	}

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		SYS_LOG_ERR("Invalid UDP data");
		goto cleanup;
	}

	/* Save the from address */
#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		net_ipaddr_copy(&net_sin6(&from_addr)->sin6_addr,
				&NET_IPV6_HDR(pkt)->src);
		net_sin6(&from_addr)->sin6_port = udp_hdr->src_port;
		net_sin6(&from_addr)->sin6_family = AF_INET6;
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		net_ipaddr_copy(&net_sin(&from_addr)->sin_addr,
				&NET_IPV4_HDR(pkt)->src);
		net_sin(&from_addr)->sin_port = udp_hdr->src_port;
		net_sin(&from_addr)->sin_family = AF_INET;
	}
#endif

	hdr_len = net_pkt_ip_hdr_len(pkt) + NET_UDPH_LEN +
			net_pkt_ipv6_ext_len(pkt);
	if (hdr_len >= net_pkt_get_len(pkt)) {
		SYS_LOG_ERR("Data not long enough");
		goto cleanup;
	}

	buf = get_buf();
	if (!buf) {
		SYS_LOG_ERR("No more message buffers available!");
		goto cleanup;
	}

	ret = net_frag_linearize(buf, MAX_PACKET_SIZE, pkt,
				 hdr_len, net_pkt_get_len(pkt) - hdr_len);
	if (ret < 0) {
		SYS_LOG_ERR("Data not long enough");
		goto cleanup;
	}

	lwm2m_udp_receive(nl_data->ctx, buf, (size_t)ret, &from_addr, false,
			  lwm2m_handle_request);

cleanup:
	if (buf) {
		free_buf(buf);
	}

	if (pkt) {
		net_pkt_unref(pkt);
	}
}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
static int setup_cert(struct net_app_ctx *app_ctx, void *cert)
{
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	struct lwm2m_dtls_data *dtls;
	struct net_layer_net_app *nl_data;

	nl_data = CONTAINER_OF(app_ctx, struct net_layer_net_app, net_app_ctx);
	if (!nl_data) {
		return -EINVAL;
	}

	dtls = nl_data->ctx->dtls_data;
	return mbedtls_ssl_conf_psk(
			&app_ctx->tls.mbedtls.conf,
			(const unsigned char *)dtls->client_psk,
			dtls->client_psk_len,
			(const unsigned char *)dtls->client_psk_id,
			dtls->client_psk_id_len);
#else
	return 0;
#endif
}
#endif /* CONFIG_NET_APP_DTLS */

int lwm2m_nl_net_app_start(struct lwm2m_ctx *client_ctx,
			   char *peer_str, u16_t peer_port)
{
	struct net_layer_net_app *nl_data;
	int ret = 0;

	nl_data = (struct net_layer_net_app *)
			lwm2m_nl_api_from_ctx(client_ctx)->nl_user_data;
	(void)memset(nl_data, 0, sizeof(*nl_data));
	nl_data->ctx = client_ctx;

	ret = net_app_init_udp_client(&nl_data->net_app_ctx,
				      &client_ctx->local_addr, NULL,
				      peer_str,
				      peer_port,
				      client_ctx->net_init_timeout,
				      client_ctx);
	if (ret) {
		SYS_LOG_ERR("net_app_init_udp_client err:%d", ret);
		goto error_start;
	}

	/* set net_app callbacks */
	ret = net_app_set_cb(&nl_data->net_app_ctx,
			     NULL, lwm2m_engine_udp_receive, NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("Could not set receive callback (err:%d)", ret);
		goto error_start;
	}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	if (client_ctx->dtls_data) {
		struct lwm2m_dtls_data *dtls = client_ctx->dtls_data;

		ret = net_app_client_tls(&nl_data->net_app_ctx,
					 dtls->dtls_result_buf,
					 dtls->dtls_result_buf_len,
					 INSTANCE_INFO,
					 strlen(INSTANCE_INFO),
					 setup_cert,
					 dtls->cert_host,
					 NULL,
					 dtls->dtls_pool,
					 dtls->dtls_stack,
					 dtls->dtls_stack_len);
		if (ret < 0) {
			SYS_LOG_ERR("Cannot init DTLS (%d)", ret);
			goto error_start;
		}
	}
#endif

	ret = net_app_connect(&nl_data->net_app_ctx,
			      client_ctx->net_timeout);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot connect UDP (%d)", ret);
		goto error_start;
	}

	/* save remote address to context */
	memcpy(&client_ctx->remote_addr,
	       &nl_data->net_app_ctx.default_ctx->remote,
	       sizeof(client_ctx->remote_addr));

	return 0;

error_start:
	net_app_close(&nl_data->net_app_ctx);
	net_app_release(&nl_data->net_app_ctx);
	return ret;
}

static struct net_layer_net_app nl_net_app_data;

static struct lwm2m_net_layer_api nl_net_app_api = {
	.nl_start     = lwm2m_nl_net_app_start,
	.nl_msg_send  = lwm2m_nl_net_app_msg_send,
	.nl_user_data = &nl_net_app_data,
};

struct lwm2m_net_layer_api *lwm2m_engine_nl_net_app_api(void)
{
	return &nl_net_app_api;
}
