/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOCKETS_TLS_H_
#define _SOCKETS_TLS_H_

#include <errno.h>

#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS

int tls_getsockopt(int sock, int level, int optname,
		   void *optval, socklen_t *optlen);
int tls_setsockopt(int sock, int level, int optname,
		   const void *optval, socklen_t optlen);

#else

static inline int tls_getsockopt(int sock, int level, int optname,
				 const void *optval, socklen_t *optlen)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(level);
	ARG_UNUSED(optname);
	ARG_UNUSED(optval);
	ARG_UNUSED(optlen);

	return -EOPNOTSUPP;
}

static inline int tls_setsockopt(int sock, int level, int optname,
				 const void *optval, socklen_t optlen)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(level);
	ARG_UNUSED(optname);
	ARG_UNUSED(optval);
	ARG_UNUSED(optlen);

	return -EOPNOTSUPP;
}

#endif

#endif
