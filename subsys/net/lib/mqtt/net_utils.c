/*
 * Copyright (c) 2017 Texas Instruments, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The following code was lifted from Zephyr's net_app library, to
 * support the mqtt port to sockets, and to decouple the mqtt lib from
 * net_app's struct net_app_ctx, which may evolve with the new
 * net_pkt/net_buf redesign.
 * This ideally could move to a generic net/utils module.
 */

#include <errno.h>
#include <stdlib.h>

#include "net_utils.h"

static int get_port_number(const char *peer_addr_str,
			   char *buf,
			   size_t buf_len)
{
	u16_t port = 0;
	char *ptr;
	int count, i;

	if (peer_addr_str[0] == '[') {
#if defined(CONFIG_NET_IPV6)
		/* IPv6 address with port number */
		int end;

		ptr = strstr(peer_addr_str, "]:");
		if (!ptr) {
			return -EINVAL;
		}

		end = min(INET6_ADDRSTRLEN, ptr - (peer_addr_str + 1));
		memcpy(buf, peer_addr_str + 1, end);
		buf[end] = '\0';

		port = strtol(ptr + 2, NULL, 10);

		return port;
#else
		return -EAFNOSUPPORT;
#endif /* CONFIG_NET_IPV6 */
	}

	count = i = 0;
	while (peer_addr_str[i]) {
		if (peer_addr_str[i] == ':') {
			count++;
		}

		i++;
	}

	if (count == 1) {
#if defined(CONFIG_NET_IPV4)
		/* IPv4 address with port number */
		int end;

		ptr = strstr(peer_addr_str, ":");
		if (!ptr) {
			return -EINVAL;
		}

		end = min(NET_IPV4_ADDR_LEN, ptr - peer_addr_str);
		memcpy(buf, peer_addr_str, end);
		buf[end] = '\0';

		port = strtol(ptr + 1, NULL, 10);

		return port;
#else
		return -EAFNOSUPPORT;
#endif /* CONFIG_NET_IPV4 */
	}

	return 0;
}

bool net_util_init_tcp_client(struct sockaddr *addr,
			      struct sockaddr *peer_addr,
			      const char *peer_addr_str,
			      u16_t peer_port)
{
	const char *base_peer_addr = peer_addr_str;
	char base_addr_str[INET6_ADDRSTRLEN + 1];
	int addr_ok = false;
	u16_t port = 0;

	/* If the peer string contains port number, use that and
	 * ignore the port number parameter.
	 */
	port = get_port_number(peer_addr_str, base_addr_str,
			       sizeof(base_addr_str));
	if (port > 0) {
		base_peer_addr = base_addr_str;
		peer_port = port;
	} else {
		strncpy(base_addr_str, peer_addr_str,
			sizeof(base_addr_str) - 1);
	}

	addr_ok = net_ipaddr_parse(base_peer_addr,
				   strlen(base_peer_addr),
				   peer_addr);

	if (addr_ok) {
#if defined(CONFIG_NET_IPV6)
		if (peer_addr->sa_family == AF_INET6) {
			net_sin6(peer_addr)->sin6_port = htons(peer_port);
		}
#endif
#if defined(CONFIG_NET_IPV4)
		if (peer_addr->sa_family == AF_INET) {
			net_sin(peer_addr)->sin_port = htons(peer_port);
		}
#endif
		addr->sa_family = peer_addr->sa_family;
	}

	return (addr_ok);
}
