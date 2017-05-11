/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/net_context.h>
#include <net/net_pkt.h>
#include <net/socket.h>

#define SET_ERRNO(x) \
	{ int _err = x; if (_err < 0) { errno = -_err; return -1; } }

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
