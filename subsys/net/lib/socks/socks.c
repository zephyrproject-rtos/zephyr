/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_socks, CONFIG_SOCKS_LOG_LEVEL);

#include <zephyr/kernel.h>
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

/* Fixed part of a CONNECT reply: VER, REP, RSV and ATYP. A domain name bound
 * address adds one length byte on top of that before the address itself.
 */
#define SOCKS5_RSP_HDR_LEN sizeof(struct socks5_command_response_common)
#define SOCKS5_RSP_PREFIX_MAX (SOCKS5_RSP_HDR_LEN + 1)

/* State of the CONNECT reply parser. The reply is not guaranteed to arrive in
 * one packet, and the packet that completes it may carry relayed data behind
 * it, so the parser has to survive across callbacks and cannot simply drop
 * whatever it did not read.
 */
struct socks5_cmd_rsp {
	uint8_t prefix[SOCKS5_RSP_PREFIX_MAX];
	uint8_t prefix_len;
	uint8_t need;			/* prefix bytes still to read */
	size_t skip;			/* bound address and port still to drop */
	bool complete;
	int status;
	struct net_pkt *leftover;	/* holds data that came in behind the reply */
};

/* Work out what is left of the reply now that its fixed part has been read.
 * The bound address is not needed for anything, so only its length matters.
 */
static void socks5_cmd_rsp_next(struct socks5_cmd_rsp *rsp)
{
	switch (rsp->prefix[3]) {
	case SOCKS5_ATYP_IPV4:
		rsp->skip = sizeof(struct socks5_ipv4_addr);
		break;
	case SOCKS5_ATYP_IPV6:
		rsp->skip = sizeof(struct socks5_ipv6_addr);
		break;
	case SOCKS5_ATYP_DOMAINNAME:
		if (rsp->prefix_len == SOCKS5_RSP_HDR_LEN) {
			/* The length byte comes next and tells us the rest */
			rsp->need = 1U;
			return;
		}

		rsp->skip = rsp->prefix[SOCKS5_RSP_HDR_LEN] + sizeof(uint16_t);
		break;
	default:
		/* Without a known address type the end of the reply cannot be
		 * found, so the rest of the stream cannot be trusted either.
		 */
		rsp->status = -EPROTO;
		return;
	}

	if (rsp->skip == 0U) {
		rsp->complete = true;
	}
}

static void socks5_cmd_rsp_cb(struct net_context *ctx,
			      struct net_pkt *pkt,
			      union net_ip_header *ip_hdr,
			      union net_proto_header *proto_hdr,
			      int status,
			      void *user_data)
{
	struct socks5_cmd_rsp *rsp = (struct socks5_cmd_rsp *)user_data;
	size_t avail;
	size_t take;

	ARG_UNUSED(ctx);
	ARG_UNUSED(ip_hdr);
	ARG_UNUSED(proto_hdr);

	if (pkt == NULL || status != 0) {
		rsp->status = (status != 0) ? status : -ECONNRESET;
		net_pkt_unref(pkt);
		return;
	}

	while (rsp->status == 0 && !rsp->complete) {
		avail = net_pkt_remaining_data(pkt);
		if (avail == 0U) {
			break;
		}

		if (rsp->need > 0U) {
			take = MIN(rsp->need, avail);

			if (net_pkt_read(pkt, &rsp->prefix[rsp->prefix_len],
					 take) < 0) {
				rsp->status = -EIO;
				break;
			}

			rsp->prefix_len += take;
			rsp->need -= take;

			if (rsp->need == 0U) {
				socks5_cmd_rsp_next(rsp);
			}

			continue;
		}

		take = MIN(rsp->skip, avail);

		if (net_pkt_skip(pkt, take) < 0) {
			rsp->status = -EIO;
			break;
		}

		rsp->skip -= take;

		if (rsp->skip == 0U) {
			rsp->complete = true;
		}
	}

	/* Whatever is left is relayed data that the proxy sent in the same
	 * segment as the reply. Hand the packet on instead of dropping it,
	 * the caller passes it to the socket receive queue.
	 */
	if (rsp->complete && net_pkt_remaining_data(pkt) > 0U) {
		rsp->leftover = pkt;
		return;
	}

	net_pkt_unref(pkt);
}

/* Build the address part of a SOCKS5 CONNECT request.
 *
 * The address type is selected from the destination, not from the proxy: the
 * request encodes the destination independently of the address family that is
 * used to reach the proxy itself.
 *
 * Return the total request length, or a negative error value.
 */
static int socks5_encode_dest(struct socks5_command_request *req,
			      const struct net_sockaddr *dest,
			      net_socklen_t dest_len)
{
	if (dest->sa_family == NET_AF_INET) {
		const struct net_sockaddr_in *d4 =
			(const struct net_sockaddr_in *)dest;

		if (dest_len < sizeof(struct net_sockaddr_in)) {
			return -EINVAL;
		}

		req->r.atyp = SOCKS5_ATYP_IPV4;

		memcpy(&req->ipv4_addr.addr, &d4->sin_addr,
		       sizeof(req->ipv4_addr.addr));

		req->ipv4_addr.port = d4->sin_port;

		return sizeof(struct socks5_command_request_common)
			+ sizeof(struct socks5_ipv4_addr);
	}

	if (dest->sa_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *d6 =
			(const struct net_sockaddr_in6 *)dest;

		if (dest_len < sizeof(struct net_sockaddr_in6)) {
			return -EINVAL;
		}

		req->r.atyp = SOCKS5_ATYP_IPV6;

		memcpy(&req->ipv6_addr.addr, &d6->sin6_addr,
		       sizeof(req->ipv6_addr.addr));

		req->ipv6_addr.port = d6->sin6_port;

		return sizeof(struct socks5_command_request_common)
			+ sizeof(struct socks5_ipv6_addr);
	}

	return -EAFNOSUPPORT;
}

static int socks5_tcp_connect(struct net_context *ctx,
			      const struct net_sockaddr *proxy,
			      net_socklen_t proxy_len,
			      const struct net_sockaddr *dest,
			      net_socklen_t dest_len,
			      struct net_pkt **leftover)
{
	struct socks5_method_request method_req;
	struct socks5_method_response method_rsp;
	struct socks5_command_request cmd_req;
	struct socks5_cmd_rsp cmd_rsp = {
		.need = SOCKS5_RSP_HDR_LEN,
	};
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
		goto error;
	}

	if (method_rsp.ver != SOCKS5_PKT_MAGIC) {
		LOG_ERR("Invalid negotiation response magic");
		ret = -EINVAL;
		goto error;
	}

	if (method_rsp.method != SOCKS5_AUTH_METHOD_NOAUTH) {
		LOG_ERR("Invalid negotiation response");
		ret = -ENOTSUP;
		goto error;
	}

	/* Negotiation complete - now connect to destination */
	cmd_req.r.ver = SOCKS5_PKT_MAGIC;
	cmd_req.r.cmd = SOCKS5_CMD_CONNECT;
	cmd_req.r.rsv = SOCKS5_PKT_RSV;

	size = socks5_encode_dest(&cmd_req, dest, dest_len);
	if (size < 0) {
		LOG_ERR("Cannot encode destination address (family %d, len %u)",
			dest->sa_family, dest_len);
		ret = size;
		goto error;
	}

	ret = net_context_sendto(ctx, (uint8_t *)&cmd_req, size,
				 proxy, proxy_len, NULL, K_NO_WAIT,
				 ctx->user_data);
	if (ret < 0) {
		LOG_ERR("Could not send CONNECT command");
		goto error;
	}

	/* The reply is not guaranteed to arrive in one packet, so keep
	 * receiving until all of it, including the bound address that is not
	 * needed, has been taken off the stream. Anything left behind would be
	 * handed to the application as if it were relayed data.
	 */
	while (!cmd_rsp.complete && cmd_rsp.status == 0) {
		ret = net_context_recv(ctx, socks5_cmd_rsp_cb,
				       K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT),
				       &cmd_rsp);
		if (ret < 0) {
			/* The packet that finished the reply may have arrived
			 * just as the wait was being armed.
			 */
			if (cmd_rsp.complete || cmd_rsp.status != 0) {
				break;
			}

			LOG_ERR("Could not receive CONNECT response");
			goto error;
		}
	}

	if (cmd_rsp.status < 0) {
		LOG_ERR("Malformed CONNECT response (%d)", cmd_rsp.status);
		ret = cmd_rsp.status;
		goto error;
	}

	if (cmd_rsp.prefix[0] != SOCKS5_PKT_MAGIC) {
		LOG_ERR("Invalid CONNECT response");
		ret = -EINVAL;
		goto error;
	}

	if (cmd_rsp.prefix[1] != SOCKS5_CMD_RESP_SUCCESS) {
		LOG_ERR("Unable to connect to destination");
		ret = -EINVAL;
		goto error;
	}

	*leftover = cmd_rsp.leftover;

	LOG_DBG("Connection through SOCKS5 proxy successful");

	return 0;

error:
	/* The response callbacks above were handed pointers to this stack
	 * frame and net_context_recv() leaves them installed. On success the
	 * caller replaces them right away, but on error nothing does, so drop
	 * them here. Otherwise closing the socket later makes the TCP stack
	 * call back into a frame that is long gone.
	 */
	(void)net_context_recv(ctx, NULL, K_NO_WAIT, NULL);

	net_pkt_unref(cmd_rsp.leftover);

	return ret;
}

int net_socks5_connect(struct net_context *ctx, const struct net_sockaddr *addr,
		       net_socklen_t addrlen, struct net_pkt **leftover)
{
	struct net_sockaddr_storage proxy;
	struct net_sockaddr *proxy_sa = net_sad(&proxy);
	net_socklen_t proxy_len = sizeof(proxy);
	int type;
	int ret;

	type = net_context_get_type(ctx);
	/* TODO: Only TCP and TLS supported, UDP and DTLS yet to support. */
	if (type != NET_SOCK_STREAM) {
		return -ENOTSUP;
	}

	ret = net_context_get_option(ctx, NET_OPT_SOCKS5, proxy_sa, &proxy_len);
	if (ret < 0) {
		return ret;
	}

	/* Connect to Proxy Server */
	ret = net_context_connect(ctx, proxy_sa, proxy_len, NULL,
				  K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT),
				  NULL);
	if (ret < 0) {
		return ret;
	}

	return socks5_tcp_connect(ctx, proxy_sa, proxy_len, addr, addrlen,
				  leftover);
}
