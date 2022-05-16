/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_socks, CONFIG_SOCKS_LOG_LEVEL);

#include <zephyr/zephyr.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_pkt.h>

#include "socks.h"
#include "socks_internal.h"

static void socks5_method_rsp_cb(struct net_context *ctx,
				 struct net_pkt *pkt,
				 union net_ip_header *ip_hdr,
				 union net_proto_header *proto_hdr,
				 int status,
				 void *user_data)
{
	struct socks5_method_response *method_rsp =
			(struct socks5_method_response *)user_data;

	if (!pkt || status) {
		memset(method_rsp, 0, sizeof(struct socks5_method_response));
		goto end;
	}

	if (net_pkt_read(pkt, (uint8_t *)method_rsp,
			 sizeof(struct socks5_method_response))) {
		memset(method_rsp, 0, sizeof(struct socks5_method_response));
	}

end:
	net_pkt_unref(pkt);
}

static void socks5_cmd_rsp_cb(struct net_context *ctx,
			      struct net_pkt *pkt,
			      union net_ip_header *ip_hdr,
			      union net_proto_header *proto_hdr,
			      int status,
			      void *user_data)
{
	struct socks5_command_response *cmd_rsp =
			(struct socks5_command_response *)user_data;
	int size;

	if (!pkt || status) {
		memset(cmd_rsp, 0,
		       sizeof(struct socks5_command_request_common));
		goto end;
	}

	size = sizeof(struct socks5_command_request_common);

	if (net_pkt_read(pkt, (uint8_t *)cmd_rsp, size)) {
		memset(cmd_rsp, 0,
		       sizeof(struct socks5_command_request_common));
	}

end:
	net_pkt_unref(pkt);
}

static int socks5_tcp_connect(struct net_context *ctx,
			      const struct sockaddr *proxy,
			      socklen_t proxy_len,
			      const struct sockaddr *dest,
			      socklen_t dest_len)
{
	struct socks5_method_request method_req;
	struct socks5_method_response method_rsp;
	struct socks5_command_request cmd_req;
	struct socks5_command_response cmd_rsp;
	int size;
	int ret;

	/* Negotiate authentication method */
	method_req.r.ver = SOCKS5_PKT_MAGIC;

	/* We only support NOAUTH at the moment */
	method_req.r.nmethods = 1U;
	method_req.methods[0] = SOCKS5_AUTH_METHOD_NOAUTH;

	/* size + 1 because just one method is supported */
	size = sizeof(struct socks5_method_request_common) + 1;

	ret = net_context_sendto(ctx, (uint8_t *)&method_req, size,
				 proxy, proxy_len, NULL, K_NO_WAIT,
				 ctx->user_data);
	if (ret < 0) {
		LOG_ERR("Could not send negotiation packet");
		return ret;
	}

	ret = net_context_recv(ctx, socks5_method_rsp_cb,
			       K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT),
			       &method_rsp);
	if (ret < 0) {
		LOG_ERR("Could not receive negotiation response");
		return ret;
	}

	if (method_rsp.ver != SOCKS5_PKT_MAGIC) {
		LOG_ERR("Invalid negotiation response magic");
		return -EINVAL;
	}

	if (method_rsp.method != SOCKS5_AUTH_METHOD_NOAUTH) {
		LOG_ERR("Invalid negotiation response");
		return -ENOTSUP;
	}

	/* Negotiation complete - now connect to destination */
	cmd_req.r.ver = SOCKS5_PKT_MAGIC;
	cmd_req.r.cmd = SOCKS5_CMD_CONNECT;
	cmd_req.r.rsv = SOCKS5_PKT_RSV;

	if (proxy->sa_family == AF_INET) {
		const struct sockaddr_in *d4 =
			(struct sockaddr_in *)dest;

		cmd_req.r.atyp = SOCKS5_ATYP_IPV4;

		memcpy(&cmd_req.ipv4_addr.addr,
		       (uint8_t *)&d4->sin_addr,
		       sizeof(cmd_req.ipv4_addr.addr));

		cmd_req.ipv4_addr.port = d4->sin_port;

		size = sizeof(struct socks5_command_request_common)
			+ sizeof(struct socks5_ipv4_addr);
	} else if (proxy->sa_family == AF_INET6) {
		const struct sockaddr_in6 *d6 =
			(struct sockaddr_in6 *)dest;

		cmd_req.r.atyp = SOCKS5_ATYP_IPV6;

		memcpy(&cmd_req.ipv6_addr.addr,
		       (uint8_t *)&d6->sin6_addr,
		       sizeof(cmd_req.ipv6_addr.addr));

		cmd_req.ipv6_addr.port = d6->sin6_port;

		size = sizeof(struct socks5_command_request_common)
			+ sizeof(struct socks5_ipv6_addr);
	}

	ret = net_context_sendto(ctx, (uint8_t *)&cmd_req, size,
				 proxy, proxy_len, NULL, K_NO_WAIT,
				 ctx->user_data);
	if (ret < 0) {
		LOG_ERR("Could not send CONNECT command");
		return ret;
	}

	ret = net_context_recv(ctx, socks5_cmd_rsp_cb,
			       K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT),
			       &cmd_rsp);
	if (ret < 0) {
		LOG_ERR("Could not receive CONNECT response");
		return ret;
	}

	if (cmd_rsp.r.ver != SOCKS5_PKT_MAGIC) {
		LOG_ERR("Invalid CONNECT response");
		return -EINVAL;
	}

	if (cmd_rsp.r.rep != SOCKS5_CMD_RESP_SUCCESS) {
		LOG_ERR("Unable to connect to destination");
		return -EINVAL;
	}

	/* Verifying the rest is not required */

	LOG_DBG("Connection through SOCKS5 proxy successful");

	return 0;
}

int net_socks5_connect(struct net_context *ctx, const struct sockaddr *addr,
		       socklen_t addrlen)
{
	struct sockaddr proxy;
	socklen_t proxy_len;
	int type;
	int ret;

	type = net_context_get_type(ctx);
	/* TODO: Only TCP and TLS supported, UDP and DTLS yet to support. */
	if (type != SOCK_STREAM) {
		return -ENOTSUP;
	}

	ret = net_context_get_option(ctx, NET_OPT_SOCKS5, &proxy, &proxy_len);
	if (ret < 0) {
		return ret;
	}

	/* Connect to Proxy Server */
	ret = net_context_connect(ctx, &proxy, proxy_len, NULL,
				  K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT),
				  NULL);
	if (ret < 0) {
		return ret;
	}

	return socks5_tcp_connect(ctx, &proxy, proxy_len, addr, addrlen);
}
