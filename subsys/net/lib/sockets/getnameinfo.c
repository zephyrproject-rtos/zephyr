/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <zephyr/net/socket.h>

int zsock_getnameinfo(const struct net_sockaddr *addr, net_socklen_t addrlen,
		      char *host, net_socklen_t hostlen,
		      char *serv, net_socklen_t servlen, int flags)
{
	/* Both net_sockaddr_in & _in6 have same offsets for family and address. */
	const struct net_sockaddr_in6 *a = (const struct net_sockaddr_in6 *)addr;

	if (host != NULL) {
		void *res = zsock_inet_ntop(a->sin6_family, &a->sin6_addr,
					    host, hostlen);

		if (res == NULL) {
			return DNS_EAI_SYSTEM;
		}
	}

	if (serv != NULL) {
		snprintk(serv, servlen, "%hu", net_ntohs(a->sin6_port));
	}

	return 0;
}
