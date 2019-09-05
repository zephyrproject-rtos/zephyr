/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_NET_SOCKETS

#include <net/socketutils.h>

const char *net_addr_str_find_port(const char *addr_str)
{
	const char *p = strrchr(addr_str, ':');

	if (p == NULL) {
		return NULL;
	}

	/* If it's not IPv6 numeric notation, we guaranteedly got a port */
	if (*addr_str != '[') {
		return p + 1;
	}

	/* IPv6 numeric address, and ':' preceded by ']' */
	if (p[-1] == ']') {
		return p + 1;
	}

	/* Otherwise, just raw IPv6 address, ':' is component separator */
	return NULL;
}

int net_getaddrinfo_addr_str(const char *addr_str, const char *def_port,
			     const struct addrinfo *hints,
			     struct addrinfo **res)
{
	const char *port;
	char host[NI_MAXHOST];

	if (addr_str == NULL) {
		errno = EINVAL;
		return -1;
	}

	port  = net_addr_str_find_port(addr_str);

	if (port == NULL) {
		port = def_port;
	} else {
		int host_len = port - addr_str - 1;

		if (host_len > sizeof(host) - 1) {
			errno = EINVAL;
			return -1;
		}
		strncpy(host, addr_str, host_len + 1);
		host[host_len] = '\0';
		addr_str = host;
	}

	return getaddrinfo(addr_str, port, hints, res);
}

#endif
