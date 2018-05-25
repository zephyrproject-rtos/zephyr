/*
 * Copyright (c) 2018 Linaro Limited
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
#include <string.h>
#include <stdlib.h>

#ifndef __ZEPHYR__

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "config.h"
#ifdef NET_LOG_ENABLED
#define NET_DBG printf
#else
#define NET_DBG(...)
#endif

#else
#include <net/net_ip.h>
/* Use the netinet/in.h standard definition: */
#define INET_ADDRSTRLEN NET_IPV4_ADDR_LEN

#endif

#include "net_utils.h"

static int get_port_number(const char *peer_addr_str,
			   char *buf,
			   size_t buf_len)
{
	uint16_t port = 0;
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

		end = min(INET_ADDRSTRLEN, ptr - peer_addr_str);
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

#ifndef __ZEPHYR__

static inline struct sockaddr_in *net_sin(const struct sockaddr *addr)
{
	return (struct sockaddr_in *)addr;
}

#if defined(CONFIG_NET_IPV6) || defined(CONFIG_NET_IPV4)
static bool convert_port(const char *buf, uint16_t *port)
{
	unsigned long tmp;
	char *endptr;

	tmp = strtoul(buf, &endptr, 10);
	if ((endptr == buf && tmp == 0) ||
	    !(*buf != '\0' && *endptr == '\0') ||
	    ((unsigned long)(unsigned short)tmp != tmp)) {
		return false;
	}

	*port = tmp;

	return true;
}
#endif /* CONFIG_NET_IPV6 || CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6)
static bool parse_ipv6(const char *str, size_t str_len,
		       struct sockaddr *addr, bool has_port)
{
	char *ptr = NULL;
	struct in6_addr *addr6;
	char ipaddr[INET6_ADDRSTRLEN + 1];
	int end, len, ret, i;
	uint16_t port;

	len = min(INET6_ADDRSTRLEN, str_len);

	for (i = 0; i < len; i++) {
		if (!str[i]) {
			len = i;
			break;
		}
	}

	if (has_port) {
		/* IPv6 address with port number */
		ptr = memchr(str, ']', len);
		if (!ptr) {
			return false;
		}

		end = min(len, ptr - (str + 1));
		memcpy(ipaddr, str + 1, end);
	} else {
		end = len;
		memcpy(ipaddr, str, end);
	}

	ipaddr[end] = '\0';

	addr6 = &net_sin6(addr)->sin6_addr;

	ret = inet_pton(AF_INET6, ipaddr, addr6);
	if (ret < 0) {
		return false;
	}

	net_sin6(addr)->sin6_family = AF_INET6;

	if (!has_port) {
		return true;
	}

	if ((ptr + 1) < (str + str_len) && *(ptr + 1) == ':') {
		len = str_len - end;

		/* Re-use the ipaddr buf for port conversion */
		memcpy(ipaddr, ptr + 2, len);
		ipaddr[len] = '\0';

		ret = convert_port(ipaddr, &port);
		if (!ret) {
			return false;
		}

		net_sin6(addr)->sin6_port = htons(port);

		NET_DBG("IPv6 host %s port %d",
			inet_ntop(AF_INET6, addr6,
				      ipaddr, sizeof(ipaddr) - 1),
			port);
	} else {
		NET_DBG("IPv6 host %s",
			inet_ntop(AF_INET6, addr6,
				      ipaddr, sizeof(ipaddr) - 1));
	}

	return true;
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
static bool parse_ipv4(const char *str, size_t str_len,
		       struct sockaddr *addr, bool has_port)
{
	char *ptr = NULL;
	char ipaddr[INET_ADDRSTRLEN + 1];
	struct in_addr *addr4;
	int end, len, ret, i;
	uint16_t port;

	len = min(INET_ADDRSTRLEN, str_len);

	for (i = 0; i < len; i++) {
		if (!str[i]) {
			len = i;
			break;
		}
	}

	if (has_port) {
		/* IPv4 address with port number */
		ptr = memchr(str, ':', len);
		if (!ptr) {
			return false;
		}

		end = min(len, ptr - str);
	} else {
		end = len;
	}

	memcpy(ipaddr, str, end);
	ipaddr[end] = '\0';

	addr4 = &net_sin(addr)->sin_addr;

	ret = inet_pton(AF_INET, ipaddr, addr4);
	if (ret < 0) {
		return false;
	}

	net_sin(addr)->sin_family = AF_INET;

	if (!has_port) {
		return true;
	}

	memcpy(ipaddr, ptr + 1, str_len - end);
	ipaddr[str_len - end] = '\0';

	ret = convert_port(ipaddr, &port);
	if (!ret) {
		return false;
	}

	net_sin(addr)->sin_port = htons(port);

	NET_DBG("IPv4 host %s port %d",
		inet_ntop(AF_INET, addr4,
			      ipaddr, sizeof(ipaddr) - 1),
		port);
	return true;
}
#endif /* CONFIG_NET_IPV4 */

bool net_ipaddr_parse(const char *str, size_t str_len, struct sockaddr *addr)
{
	int i, count;

	if (!str || str_len == 0) {
		return false;
	}

	/* We cannot accept empty string here */
	if (*str == '\0') {
		return false;
	}

	if (*str == '[') {
#if defined(CONFIG_NET_IPV6)
		return parse_ipv6(str, str_len, addr, true);
#else
		return false;
#endif /* CONFIG_NET_IPV6 */
	}

	for (count = i = 0; str[i] && i < str_len; i++) {
		if (str[i] == ':') {
			count++;
		}
	}

	if (count == 1) {
#if defined(CONFIG_NET_IPV4)
		return parse_ipv4(str, str_len, addr, true);
#else
		return false;
#endif /* CONFIG_NET_IPV4 */
	}

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	if (!parse_ipv4(str, str_len, addr, false)) {
		return parse_ipv6(str, str_len, addr, false);
	}

	return true;
#endif

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	return parse_ipv4(str, str_len, addr, false);
#endif

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
	return parse_ipv6(str, str_len, addr, false);
#endif
}
#endif  /* !__ZEPHYR__ */

bool net_util_init_tcp_client(struct sockaddr *addr,
			      struct sockaddr *peer_addr,
			      const char *peer_addr_str,
			      uint16_t peer_port)
{
	const char *base_peer_addr = peer_addr_str;
	char base_addr_str[INET6_ADDRSTRLEN + 1];
	int addr_ok = false;
	uint16_t port = 0;

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
			((struct sockaddr_in6 *)peer_addr)->sin6_port =
				htons(peer_port);
		}
#endif
#if defined(CONFIG_NET_IPV4)
		if (peer_addr->sa_family == AF_INET) {
			((struct sockaddr_in *)peer_addr)->sin_port =
				htons(peer_port);
		}
#endif
		addr->sa_family = peer_addr->sa_family;
	}

	return (addr_ok);
}
