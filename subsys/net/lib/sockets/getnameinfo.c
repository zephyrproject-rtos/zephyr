/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <net/socket.h>

int zsock_getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
		      char *host, socklen_t hostlen,
		      char *serv, socklen_t servlen, int flags)
{
	/* Both sockaddr_in & _in6 have same offsets for family and address. */
	struct sockaddr_in *a = (struct sockaddr_in *)addr;

	if (host != NULL) {
		void *res = zsock_inet_ntop(a->sin_family, &a->sin_addr,
					    host, hostlen);

		if (res == NULL) {
			return DNS_EAI_SYSTEM;
		}
	}

	if (serv != NULL) {
		snprintk(serv, servlen, "%hu", ntohs(a->sin_port));
	}

	return 0;
}
