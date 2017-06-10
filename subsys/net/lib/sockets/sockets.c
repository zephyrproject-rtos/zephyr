/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_SOCKETS)
#define SYS_LOG_DOMAIN "net/sock"
#define NET_LOG_ENABLED 1
#endif

#include <net/net_context.h>
#include <net/net_pkt.h>
#include <net/socket.h>

#define SET_ERRNO(x) \
	{ int _err = x; if (_err < 0) { errno = -_err; return -1; } }

#define sock_is_eof(ctx) ((ctx)->user_data != NULL)
#define sock_set_eof(ctx) { (ctx)->user_data = INT_TO_POINTER(1); }

int zsock_socket(int family, int type, int proto)
{
	struct net_context *ctx;

	SET_ERRNO(net_context_get(family, type, proto, &ctx));
	/* recv_q and accept_q are in union */
	k_fifo_init(&ctx->recv_q);

	/* TODO: Ensure non-negative */
	return POINTER_TO_INT(ctx);
}

int zsock_close(int sock)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_put(ctx));
	return 0;
}

static void zsock_accepted_cb(struct net_context *new_ctx,
			      struct sockaddr *addr, socklen_t addrlen,
			      int status, void *user_data) {
	struct net_context *parent = user_data;

	NET_DBG("parent=%p, ctx=%p, st=%d", parent, new_ctx, status);

	k_fifo_put(&parent->accept_q, new_ctx);
}

static void zsock_received_cb(struct net_context *ctx, struct net_pkt *pkt,
			      int status, void *user_data) {
	unsigned int header_len;

	NET_DBG("ctx=%p, pkt=%p, st=%d, user_data=%p", ctx, pkt, status,
		user_data);

	/* if pkt is NULL, EOF */
	if (!pkt) {
		struct net_pkt *last_pkt = k_fifo_peek_tail(&ctx->recv_q);

		if (!last_pkt) {
			/* If there're no packets in the queue, recv() may
			 * be blocked waiting on it to become non-empty,
			 * so cancel that wait.
			 */
			sock_set_eof(ctx);
			k_fifo_cancel_wait(&ctx->recv_q);
			NET_DBG("Marked socket %p as peer-closed", ctx);
		} else {
			net_pkt_set_eof(last_pkt, true);
			NET_DBG("Set EOF flag on pkt %p", ctx);
		}
		return;
	}

	/* Normal packet */
	net_pkt_set_eof(pkt, false);

	/* We don't care about packet header, so get rid of it asap */
	header_len = net_pkt_appdata(pkt) - pkt->frags->data;
	net_buf_pull(pkt->frags, header_len);

	k_fifo_put(&ctx->recv_q, pkt);
}

int zsock_bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_bind(ctx, addr, addrlen));
	/* For DGRAM socket, we expect to receive packets after call to
	 * bind(), but for STREAM socket, next expected operation is
	 * listen(), which doesn't work if recv callback is set.
	 */
	if (net_context_get_type(ctx) == SOCK_DGRAM) {
		SET_ERRNO(net_context_recv(ctx, zsock_received_cb, K_NO_WAIT,
					   NULL));
	}

	return 0;
}

int zsock_connect(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_connect(ctx, addr, addrlen, NULL, K_FOREVER,
				      NULL));
	SET_ERRNO(net_context_recv(ctx, zsock_received_cb, K_NO_WAIT, NULL));

	return 0;
}

int zsock_listen(int sock, int backlog)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_listen(ctx, backlog));
	SET_ERRNO(net_context_accept(ctx, zsock_accepted_cb, K_NO_WAIT, ctx));

	return 0;
}

int zsock_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	struct net_context *parent = INT_TO_POINTER(sock);

	struct net_context *ctx = k_fifo_get(&parent->accept_q, K_FOREVER);

	SET_ERRNO(net_context_recv(ctx, zsock_received_cb, K_NO_WAIT, NULL));

	if (addr != NULL && addrlen != NULL) {
		int len = min(*addrlen, sizeof(ctx->remote));

		memcpy(addr, &ctx->remote, len);
		*addrlen = sizeof(ctx->remote);
	}

	/* TODO: Ensure non-negative */
	return POINTER_TO_INT(ctx);
}

ssize_t zsock_send(int sock, const void *buf, size_t len, int flags)
{
	ARG_UNUSED(flags);
	int err;
	struct net_context *ctx = INT_TO_POINTER(sock);
	struct net_pkt *send_pkt = net_pkt_get_tx(ctx, K_FOREVER);
	size_t max_len = net_if_get_mtu(net_context_get_iface(ctx));

	/* Make sure we don't send more data in one packet than
	 * MTU allows. Optimize for number of branches in the code.
	 */
	max_len -= NET_IPV4TCPH_LEN;
	if (net_context_get_family(ctx) != AF_INET) {
		max_len -= NET_IPV6TCPH_LEN - NET_IPV4TCPH_LEN;
	}

	if (len > max_len) {
		len = max_len;
	}

	len = net_pkt_append(send_pkt, len, buf, K_FOREVER);
	err = net_context_send(send_pkt, /*cb*/NULL, K_FOREVER, NULL, NULL);
	if (err < 0) {
		net_pkt_unref(send_pkt);
		errno = -err;
		return -1;
	}

	return len;
}
