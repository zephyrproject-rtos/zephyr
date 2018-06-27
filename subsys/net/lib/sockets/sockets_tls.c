/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/net_context.h>
#include <net/socket.h>

#include "sockets_tls.h"

int tls_getsockopt(int sock, int level, int optname,
		   void *optval, socklen_t *optlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);
	int ret = 0;

	if (net_context_get_type(ctx) != SOCK_STREAM) {
		return -EBADF;
	}

	if  (!optval || !optlen || !*optlen) {
		return -EFAULT;
	}

	switch (optname) {
	case TLS_ENABLE:
		if (*optlen != sizeof(int)) {
			return -EINVAL;
		}

		if (net_context_get_option(ctx, NET_OPT_TLS_ENABLE,
					   optval, optlen) < 0) {
			ret = -EINVAL;
		}

		break;

	case TLS_SEC_TAG_LIST:
		if (net_context_get_option(ctx, NET_OPT_TLS_SEC_TAG_LIST,
					   optval, optlen) < 0) {
			ret = -EINVAL;
		}

		break;

	default:
		return -ENOPROTOOPT;
	}

	return ret;
}

int tls_setsockopt(int sock, int level, int optname,
		   const void *optval, socklen_t optlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);
	int ret = 0;

	if (net_context_get_type(ctx) != SOCK_STREAM) {
		return -EBADF;
	}

	if  (!optval || !optlen) {
		return -EFAULT;
	}

	switch (optname) {
	case TLS_ENABLE:
		if (optlen != sizeof(int)) {
			return -EINVAL;
		}

		if (net_context_set_option(ctx, NET_OPT_TLS_ENABLE,
					   optval, optlen) < 0) {
			ret = -EINVAL;
		}

		break;

	case TLS_SEC_TAG_LIST:
		if (net_context_set_option(ctx, NET_OPT_TLS_SEC_TAG_LIST,
					   optval, optlen) < 0) {
			ret = -EINVAL;
		}

		break;

	default:
		return -ENOPROTOOPT;
	}

	return ret;
}
