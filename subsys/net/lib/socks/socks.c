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

BUILD_ASSERT(sizeof(struct socks5_method_response) ==
	     SIZEOF_FIELD(struct net_context, socks5.method_rsp));

/* Where the handshake stands. The method callback moves it on to the CONNECT
 * reply once the method selection reply is in, and the command callback moves
 * it on from pending once the CONNECT reply is complete or broken. Both use a
 * compare-and-swap, so a callback that runs after socks5_tcp_connect() has
 * closed the handshake leaves it closed. If the wait for the CONNECT reply
 * expires, socks5_tcp_connect() closes it with a compare-and-swap as well, so
 * exactly one of them settles the reply and the other one goes along with it.
 */
enum {
	SOCKS5_CMD_RSP_PENDING,		/* still reading the reply */
	SOCKS5_CMD_RSP_RELAYING,	/* reply accepted, pass the rest on */
	SOCKS5_CMD_RSP_CLOSED,		/* attempt failed, drop the rest */
	SOCKS5_METHOD_RSP_PENDING,	/* waiting for the method selection */
};

static void socks5_method_rsp_cb(struct net_context *ctx,
				 struct net_pkt *pkt,
				 union net_ip_header *ip_hdr,
				 union net_proto_header *proto_hdr,
				 int status,
				 void *user_data)
{
	struct socks5_method_response *method_rsp =
			(struct socks5_method_response *)ctx->socks5.method_rsp;
	atomic_t *state = &ctx->socks5.cmd_rsp.state;

	ARG_UNUSED(user_data);

	/* Only the first reply counts, and only while it is waited for */
	if (atomic_get(state) != SOCKS5_METHOD_RSP_PENDING) {
		goto end;
	}

	if (!pkt || status) {
		memset(method_rsp, 0, sizeof(struct socks5_method_response));
		goto settle;
	}

	if (net_pkt_read(pkt, (uint8_t *)method_rsp,
			 sizeof(struct socks5_method_response))) {
		memset(method_rsp, 0, sizeof(struct socks5_method_response));
	}

settle:
	/* socks5_tcp_connect() may have stopped waiting already */
	if (atomic_cas(state, SOCKS5_METHOD_RSP_PENDING,
		       SOCKS5_CMD_RSP_PENDING)) {
		k_sem_give(&ctx->socks5.cmd_rsp.done);
	}

end:
	net_pkt_unref(pkt);
}

/* Fixed part of a CONNECT reply: VER, REP, RSV and ATYP. A domain name bound
 * address adds one length byte on top of that before the address itself.
 */
#define SOCKS5_RSP_HDR_LEN sizeof(struct socks5_command_response_common)
#define SOCKS5_RSP_PREFIX_MAX (SOCKS5_RSP_HDR_LEN + 1)

BUILD_ASSERT(SIZEOF_FIELD(struct net_socks5_cmd_rsp, prefix) ==
	     SOCKS5_RSP_PREFIX_MAX);

/* Work out what is left of the reply now that its fixed part has been read.
 * The bound address is not needed for anything, so only its length matters.
 */
static void socks5_cmd_rsp_next(struct net_socks5_cmd_rsp *rsp)
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
	struct net_socks5_cmd_rsp *rsp = &ctx->socks5.cmd_rsp;
	atomic_val_t next;
	size_t avail;
	size_t take;

	ARG_UNUSED(user_data);

	switch (atomic_get(&rsp->state)) {
	case SOCKS5_CMD_RSP_RELAYING:
		/* The reply has been accepted, so this is relayed data or the
		 * end of the stream. The caller may not have installed its own
		 * callback yet, or did so after this call had already started.
		 * The receive path makes these calls one at a time, so passing
		 * everything on from here keeps it in order.
		 */
		ctx->socks5.recv_cb(ctx, pkt, ip_hdr, proto_hdr, status,
				    ctx->socks5.recv_user_data);
		return;
	case SOCKS5_CMD_RSP_CLOSED:
		net_pkt_unref(pkt);
		return;
	default:
		break;
	}

	if (pkt == NULL || status != 0) {
		rsp->status = (status != 0) ? status : -ECONNRESET;
		goto settle;
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

	if (rsp->status == 0 && !rsp->complete) {
		/* The rest of the reply is still to come */
		net_pkt_unref(pkt);
		return;
	}

settle:
	if (rsp->status == 0 && rsp->prefix[0] == SOCKS5_PKT_MAGIC &&
	    rsp->prefix[1] == SOCKS5_CMD_RESP_SUCCESS) {
		next = SOCKS5_CMD_RSP_RELAYING;
	} else {
		next = SOCKS5_CMD_RSP_CLOSED;
	}

	/* socks5_tcp_connect() may have stopped waiting already, in which
	 * case the connection is reported as failed and nothing is relayed.
	 */
	if (!atomic_cas(&rsp->state, SOCKS5_CMD_RSP_PENDING, next)) {
		next = SOCKS5_CMD_RSP_CLOSED;
	} else {
		k_sem_give(&rsp->done);
	}

	/* Whatever is left is relayed data that the proxy sent in the same
	 * segment as the reply. Pass it on only after the waiting thread has
	 * been woken up: the callback may need a lock that the caller of
	 * net_socks5_connect() holds until the call returns.
	 */
	if (next == SOCKS5_CMD_RSP_RELAYING &&
	    net_pkt_remaining_data(pkt) > 0U) {
		ctx->socks5.recv_cb(ctx, pkt, ip_hdr, proto_hdr, 0,
				    ctx->socks5.recv_user_data);
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
			      net_context_recv_cb_t cb,
			      void *user_data)
{
	struct socks5_method_request method_req;
	struct socks5_method_response *method_rsp =
			(struct socks5_method_response *)ctx->socks5.method_rsp;
	struct socks5_command_request cmd_req;
	struct net_socks5_cmd_rsp *cmd_rsp = &ctx->socks5.cmd_rsp;
	int size;
	int ret;

	memset(&ctx->socks5, 0, sizeof(ctx->socks5));
	cmd_rsp->need = SOCKS5_RSP_HDR_LEN;
	k_sem_init(&cmd_rsp->done, 0, 1);
	ctx->socks5.recv_cb = cb;
	ctx->socks5.recv_user_data = user_data;
	atomic_set(&cmd_rsp->state, SOCKS5_METHOD_RSP_PENDING);

	/* Negotiate authentication method */
	method_req.r.ver = SOCKS5_PKT_MAGIC;

	/* We only support NOAUTH at the moment */
	method_req.r.nmethods = 1U;
	method_req.methods[0] = SOCKS5_AUTH_METHOD_NOAUTH;

	/* size + 1 because just one method is supported */
	size = sizeof(struct socks5_method_request_common) + 1;

	/* Each callback is installed before its request is sent. A reply that
	 * arrives first would otherwise be dropped, or be taken by the
	 * callback of the previous step, and the wait for it would time out.
	 */
	ret = net_context_recv(ctx, socks5_method_rsp_cb, K_NO_WAIT, NULL);
	if (ret < 0) {
		LOG_ERR("Could not receive negotiation response");
		goto error;
	}

	ret = net_context_sendto(ctx, (uint8_t *)&method_req, size,
				 proxy, proxy_len, NULL, K_NO_WAIT,
				 ctx->user_data);
	if (ret < 0) {
		LOG_ERR("Could not send negotiation packet");
		goto error;
	}

	if (k_sem_take(&cmd_rsp->done,
		       K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT)) < 0) {
		LOG_ERR("Could not receive negotiation response");
		ret = -ETIMEDOUT;
		goto error;
	}

	if (method_rsp->ver != SOCKS5_PKT_MAGIC) {
		LOG_ERR("Invalid negotiation response magic");
		ret = -EINVAL;
		goto error;
	}

	if (method_rsp->method != SOCKS5_AUTH_METHOD_NOAUTH) {
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

	ret = net_context_recv(ctx, socks5_cmd_rsp_cb, K_NO_WAIT, NULL);
	if (ret < 0) {
		LOG_ERR("Could not receive CONNECT response");
		goto error;
	}

	ret = net_context_sendto(ctx, (uint8_t *)&cmd_req, size,
				 proxy, proxy_len, NULL, K_NO_WAIT,
				 ctx->user_data);
	if (ret < 0) {
		LOG_ERR("Could not send CONNECT command");
		goto error;
	}

	/* The reply is not guaranteed to arrive in one packet, so the command
	 * callback keeps reading until all of it, including the bound address
	 * that is not needed, has been taken off the stream. Anything left
	 * behind would be handed to the application as if it were relayed
	 * data. The callback signals once the reply is settled and then passes
	 * on whatever follows it, which may block until this call returns, so
	 * wait for the signal and not for the callback to return.
	 */
	if (k_sem_take(&cmd_rsp->done,
		       K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT)) < 0 &&
	    atomic_cas(&cmd_rsp->state, SOCKS5_CMD_RSP_PENDING,
		       SOCKS5_CMD_RSP_CLOSED)) {
		/* Otherwise the reply was settled just as the wait expired */
		LOG_ERR("Could not receive CONNECT response");
		ret = -ETIMEDOUT;
		goto error;
	}

	if (cmd_rsp->status < 0) {
		LOG_ERR("Malformed CONNECT response (%d)", cmd_rsp->status);
		ret = cmd_rsp->status;
		goto error;
	}

	if (cmd_rsp->prefix[0] != SOCKS5_PKT_MAGIC) {
		LOG_ERR("Invalid CONNECT response");
		ret = -EINVAL;
		goto error;
	}

	if (cmd_rsp->prefix[1] != SOCKS5_CMD_RESP_SUCCESS) {
		LOG_ERR("Unable to connect to destination");
		ret = -EINVAL;
		goto error;
	}

	/* The command callback has accepted the reply as well and stays
	 * installed until the caller replaces it, passing everything that
	 * arrives in the meantime on to cb.
	 */
	LOG_DBG("Connection through SOCKS5 proxy successful");

	return 0;

error:
	/* net_context_recv() leaves the response callbacks installed. On
	 * success the caller replaces them, but on error nothing does, so drop
	 * them here. That only stops future calls: one that is already running
	 * still finishes, which is why the state it writes is kept in the
	 * context and not on this stack. Close the handshake as well, so that
	 * such a call drops what it gets instead of acting on it.
	 */
	(void)net_context_recv(ctx, NULL, K_NO_WAIT, NULL);
	atomic_set(&cmd_rsp->state, SOCKS5_CMD_RSP_CLOSED);

	return ret;
}

int net_socks5_connect(struct net_context *ctx, const struct net_sockaddr *addr,
		       net_socklen_t addrlen, net_context_recv_cb_t cb,
		       void *user_data)
{
	struct net_sockaddr_storage proxy = { 0 };
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
				  cb, user_data);
}
