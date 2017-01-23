/** @file
 * @brief Generic connection related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_CONN)
#define SYS_LOG_DOMAIN "net/conn"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <misc/util.h>

#include <net/net_core.h>
#include <net/nbuf.h>

#include "net_private.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "connection.h"
#include "net_stats.h"

/** Is this connection used or not */
#define NET_CONN_IN_USE BIT(0)

/** Remote address set */
#define NET_CONN_REMOTE_ADDR_SET BIT(1)

/** Local address set */
#define NET_CONN_LOCAL_ADDR_SET BIT(2)

/** Rank bits */
#define NET_RANK_LOCAL_PORT         BIT(0)
#define NET_RANK_REMOTE_PORT        BIT(1)
#define NET_RANK_LOCAL_UNSPEC_ADDR  BIT(2)
#define NET_RANK_REMOTE_UNSPEC_ADDR BIT(3)
#define NET_RANK_LOCAL_SPEC_ADDR    BIT(4)
#define NET_RANK_REMOTE_SPEC_ADDR   BIT(5)

static struct net_conn conns[CONFIG_NET_MAX_CONN];

/* This is only used for getting source and destination ports. Because
 * both TCP and UDP header have these in the same location, we can check
 * them both using the UDP struct.
 */
#define NET_CONN_BUF(buf) ((struct net_udp_hdr *)(net_nbuf_udp_data(buf)))

#if defined(CONFIG_NET_DEBUG_CONN)
static inline const char *proto2str(enum net_ip_protocol proto)
{
	switch (proto) {
	case IPPROTO_ICMP:
		return "ICMPv4";
	case IPPROTO_TCP:
		return "TCP";
	case IPPROTO_UDP:
		return "UDP";
	case IPPROTO_ICMPV6:
		return "ICMPv6";
	default:
		break;
	}

	return "<unknown>";
}
#endif /* CONFIG_NET_DEBUG_CONN */

#if defined(CONFIG_NET_CONN_CACHE)

/* Cache the connection so that we do not have to go
 * through the full list of connections when receiving
 * a network packet. The cache contains an index to
 * corresponding entry in conns array.
 *
 * Hash value is constructed like this:
 *
 *   bit      description
 *   0  - 3   bits from remote port
 *   4  - 7   bits from local port
 *   8  - 18  bits from remote address
 *   19 - 29  bits from local address
 *   30       family
 *   31       protocol
 */
struct conn_hash {
	uint32_t value;
	int32_t idx;
};

struct conn_hash_neg {
	uint32_t value;
};

/** Connection cache */
static struct conn_hash conn_cache[CONFIG_NET_MAX_CONN];

/** Negative cache, we definitely do not have a connection
 * to these hosts.
 */
static struct conn_hash_neg conn_cache_neg[CONFIG_NET_MAX_CONN];

#define TAKE_BIT(val, bit, max, used)				\
	(((val & BIT(bit)) >> bit) << (max - used))

static inline uint8_t ports_to_hash(uint16_t remote_port,
				    uint16_t local_port)
{
	/* Note that we do not convert port value to network byte order */
	return (remote_port & BIT(0)) |
		((remote_port & BIT(4)) >> 3) |
		((remote_port & BIT(8)) >> 6) |
		((remote_port & BIT(15)) >> 12) |
		(((local_port & BIT(0)) |
		  ((local_port & BIT(4)) >> 3) |
		  ((local_port & BIT(8)) >> 6) |
		  ((local_port & BIT(15)) >> 12)) << 4);
}

static inline uint16_t ipv6_to_hash(struct in6_addr *addr)
{
	/* There is 11 bits available for IPv6 address */
	/* Use more bits from the lower part of address space */
	return
		/* Take 3 bits from higher values */
		TAKE_BIT(addr->s6_addr32[0], 31, 11, 1) |
		TAKE_BIT(addr->s6_addr32[0], 15, 11, 2) |
		TAKE_BIT(addr->s6_addr32[0], 7, 11, 3) |

		/* Take 2 bits from higher middle values */
		TAKE_BIT(addr->s6_addr32[1], 31, 11, 4) |
		TAKE_BIT(addr->s6_addr32[1], 15, 11, 5) |

		/* Take 2 bits from lower middle values */
		TAKE_BIT(addr->s6_addr32[2], 31, 11, 6) |
		TAKE_BIT(addr->s6_addr32[2], 15, 11, 7) |

		/* Take 4 bits from lower values */
		TAKE_BIT(addr->s6_addr32[3], 31, 11, 8) |
		TAKE_BIT(addr->s6_addr32[3], 15, 11, 9) |
		TAKE_BIT(addr->s6_addr32[3], 7, 11, 10) |
		TAKE_BIT(addr->s6_addr32[3], 0, 11, 11);
}

static inline uint16_t ipv4_to_hash(struct in_addr *addr)
{
	/* There is 11 bits available for IPv4 address */
	/* Use more bits from the lower part of address space */
	return
		TAKE_BIT(addr->s_addr[0], 31, 11, 1) |
		TAKE_BIT(addr->s_addr[0], 27, 11, 2) |
		TAKE_BIT(addr->s_addr[0], 21, 11, 3) |
		TAKE_BIT(addr->s_addr[0], 17, 11, 4) |
		TAKE_BIT(addr->s_addr[0], 14, 11, 5) |
		TAKE_BIT(addr->s_addr[0], 11, 11, 6) |
		TAKE_BIT(addr->s_addr[0], 8, 11, 7) |
		TAKE_BIT(addr->s_addr[0], 5, 11, 8) |
		TAKE_BIT(addr->s_addr[0], 3, 11, 9) |
		TAKE_BIT(addr->s_addr[0], 2, 11, 10) |
		TAKE_BIT(addr->s_addr[0], 0, 11, 11);
}

/* Return either the first free position in the cache (idx < 0) or
 * the existing cached position (idx >= 0)
 */
static int32_t check_hash(enum net_ip_protocol proto,
			  sa_family_t family,
			  void *remote_addr,
			  void *local_addr,
			  uint16_t remote_port,
			  uint16_t local_port,
			  uint32_t *cache_value)
{
	int i, free_pos = -1;
	uint32_t value = 0;

	value = ports_to_hash(remote_port, local_port);

#if defined(CONFIG_NET_UDP)
	if (proto == IPPROTO_UDP) {
		value |= BIT(31);
	}
#endif

#if defined(CONFIG_NET_TCP)
	if (proto == IPPROTO_TCP) {
		value &= ~BIT(31);
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		value |= BIT(30);
		value |= ipv6_to_hash((struct in6_addr *)remote_addr) << 8;
		value |= ipv6_to_hash((struct in6_addr *)local_addr) << 19;
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		value &= ~BIT(30);
		value |= ipv4_to_hash((struct in_addr *)remote_addr) << 8;
		value |= ipv4_to_hash((struct in_addr *)local_addr) << 19;
	}
#endif

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (conn_cache[i].value == value) {
			return i;
		}

		if (conn_cache[i].idx < 0 && free_pos < 0) {
			free_pos = i;
		}
	}

	if (free_pos >= 0) {
		conn_cache[free_pos].value = value;
		return free_pos;
	}

	*cache_value = value;

	return -ENOENT;
}

static inline int32_t get_conn(enum net_ip_protocol proto,
			       sa_family_t family,
			       struct net_buf *buf,
			       uint32_t *cache_value)
{
#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		return check_hash(proto, family,
				  &NET_IPV4_BUF(buf)->src,
				  &NET_IPV4_BUF(buf)->dst,
				  NET_UDP_BUF(buf)->src_port,
				  NET_UDP_BUF(buf)->dst_port,
				  cache_value);
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		return check_hash(proto, family,
				  &NET_IPV6_BUF(buf)->src,
				  &NET_IPV6_BUF(buf)->dst,
				  NET_UDP_BUF(buf)->src_port,
				  NET_UDP_BUF(buf)->dst_port,
				  cache_value);
	}
#endif

	return -1;
}

static inline void cache_add_neg(uint32_t cache_value)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONN && cache_value > 0; i++) {
		if (conn_cache_neg[i].value) {
			continue;
		}

		NET_DBG("Add to neg cache value 0x%x", cache_value);

		conn_cache_neg[i].value = cache_value;
		break;
	}
}

static inline bool cache_check_neg(uint32_t cache_value)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONN && cache_value > 0; i++) {
		if (conn_cache_neg[i].value == cache_value) {
			NET_DBG("Cache neg [%d] value 0x%x found",
				i, cache_value);
			return true;
		}
	}

	return false;
}

static void cache_clear(void)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		conn_cache[i].idx = -1;
		conn_cache_neg[i].value = 0;
	}
}

static inline enum net_verdict cache_check(enum net_ip_protocol proto,
					   struct net_buf *buf,
					   uint32_t *cache_value,
					   int32_t *pos)
{
	*pos = get_conn(proto, net_nbuf_family(buf), buf, cache_value);
	if (*pos >= 0) {
		if (conn_cache[*pos].idx >= 0) {
			/* Connection is in the cache */
			struct net_conn *conn;

			conn = &conns[conn_cache[*pos].idx];

			NET_DBG("Cache %s listener for buf %p src port %u "
				"dst port %u family %d cache[%d] 0x%x",
				proto2str(proto), buf,
				ntohs(NET_CONN_BUF(buf)->src_port),
				ntohs(NET_CONN_BUF(buf)->dst_port),
				net_nbuf_family(buf), *pos,
				conn_cache[*pos].value);

			return conn->cb(conn, buf, conn->user_data);
		}
	} else if (*cache_value > 0) {
		if (cache_check_neg(*cache_value)) {
			NET_DBG("Drop by cache");
			return NET_DROP;
		}
	}

	return NET_CONTINUE;
}
#else
#define cache_clear(...)
#define cache_add_neg(...)
#define cache_check(...) NET_CONTINUE
#endif /* CONFIG_NET_CONN_CACHE */

int net_conn_unregister(struct net_conn_handle *handle)
{
	struct net_conn *conn = (struct net_conn *)handle;

	if (conn < &conns[0] || conn > &conns[CONFIG_NET_MAX_CONN]) {
		return -EINVAL;
	}

	if (!(conn->flags & NET_CONN_IN_USE)) {
		return -ENOENT;
	}

	NET_DBG("[%zu] connection handler %p removed",
		(conn - conns) / sizeof(*conn), conn);

	conn->flags = 0;

	return 0;
}

int net_conn_change_callback(struct net_conn_handle *handle,
			     net_conn_cb_t cb, void *user_data)
{
	struct net_conn *conn = (struct net_conn *)handle;

	if (conn < &conns[0] || conn > &conns[CONFIG_NET_MAX_CONN]) {
		return -EINVAL;
	}

	if (!(conn->flags & NET_CONN_IN_USE)) {
		return -ENOENT;
	}

	NET_DBG("[%zu] connection handler %p changed callback",
		(conn - conns) / sizeof(*conn), conn);

	conn->cb = cb;
	conn->user_data = user_data;

	return 0;
}

#if defined(CONFIG_NET_DEBUG_CONN)
static inline
void prepare_register_debug_print(char *dst, int dst_len,
				  char *src, int src_len,
				  const struct sockaddr *remote_addr,
				  const struct sockaddr *local_addr)
{
	if (remote_addr && remote_addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		snprintk(dst, dst_len, "%s",
			 net_sprint_ipv6_addr(&net_sin6(remote_addr)->
							sin6_addr));
#else
		snprintk(dst, dst_len, "%s", "?");
#endif

	} else if (remote_addr && remote_addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		snprintk(dst, dst_len, "%s",
			 net_sprint_ipv4_addr(&net_sin(remote_addr)->
							sin_addr));
#else
		snprintk(dst, dst_len, "%s", "?");
#endif

	} else {
		snprintk(dst, dst_len, "%s", "-");
	}

	if (local_addr && local_addr->family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		snprintk(src, src_len, "%s",
			 net_sprint_ipv6_addr(&net_sin6(local_addr)->
							sin6_addr));
#else
		snprintk(src, src_len, "%s", "?");
#endif

	} else if (local_addr && local_addr->family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		snprintk(src, src_len, "%s",
			 net_sprint_ipv4_addr(&net_sin(local_addr)->
							sin_addr));
#else
		snprintk(src, src_len, "%s", "?");
#endif

	} else {
		snprintk(src, src_len, "%s", "-");
	}
}
#endif /* CONFIG_NET_DEBUG_CONN */

int net_conn_register(enum net_ip_protocol proto,
		      const struct sockaddr *remote_addr,
		      const struct sockaddr *local_addr,
		      uint16_t remote_port,
		      uint16_t local_port,
		      net_conn_cb_t cb,
		      void *user_data,
		      struct net_conn_handle **handle)
{
	int i;
	uint8_t rank = 0;

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (conns[i].flags & NET_CONN_IN_USE) {
			continue;
		}

		if (remote_addr) {
			if (remote_addr->family != AF_INET &&
			    remote_addr->family != AF_INET6) {
				NET_DBG("Remote address family not set.");
				return -EINVAL;
			}

			conns[i].flags |= NET_CONN_REMOTE_ADDR_SET;

			memcpy(&conns[i].remote_addr, remote_addr,
			       sizeof(struct sockaddr));

#if defined(CONFIG_NET_IPV6)
			if (remote_addr->family == AF_INET6) {
				if (net_is_ipv6_addr_unspecified(
					    &net_sin6(remote_addr)->
							sin6_addr)) {
					rank |= NET_RANK_REMOTE_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_REMOTE_SPEC_ADDR;
				}
			}
#endif

#if defined(CONFIG_NET_IPV4)
			if (remote_addr->family == AF_INET) {
				if (!net_sin(remote_addr)->
							sin_addr.s_addr[0]) {
					rank |= NET_RANK_REMOTE_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_REMOTE_SPEC_ADDR;
				}
			}
#endif
		}

		if (local_addr) {
			if (local_addr->family != AF_INET &&
			    local_addr->family != AF_INET6) {
				NET_DBG("Local address family not set.");
				return -EINVAL;
			}

			conns[i].flags |= NET_CONN_LOCAL_ADDR_SET;

			memcpy(&conns[i].local_addr, local_addr,
			       sizeof(struct sockaddr));

#if defined(CONFIG_NET_IPV6)
			if (local_addr->family == AF_INET6) {
				if (net_is_ipv6_addr_unspecified(
					    &net_sin6(local_addr)->
							sin6_addr)) {
					rank |= NET_RANK_LOCAL_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_LOCAL_SPEC_ADDR;
				}
			}
#endif

#if defined(CONFIG_NET_IPV4)
			if (local_addr->family == AF_INET) {
				if (!net_sin(local_addr)->sin_addr.s_addr[0]) {
					rank |= NET_RANK_LOCAL_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_LOCAL_SPEC_ADDR;
				}
			}
#endif
		}

		if (remote_addr && local_addr) {
			if (remote_addr->family != local_addr->family) {
				NET_DBG("Address families different.");
				return -EINVAL;
			}
		}

		if (remote_port) {
			rank |= NET_RANK_REMOTE_PORT;
			net_sin(&conns[i].remote_addr)->sin_port =
				htons(remote_port);
		}

		if (local_port) {
			rank |= NET_RANK_LOCAL_PORT;
			net_sin(&conns[i].local_addr)->sin_port =
				htons(local_port);
		}

		conns[i].flags |= NET_CONN_IN_USE;
		conns[i].cb = cb;
		conns[i].user_data = user_data;
		conns[i].rank = rank;
		conns[i].proto = proto;

		/* Cache needs to be cleared if new entries are added. */
		cache_clear();

#if defined(CONFIG_NET_DEBUG_CONN)
		do {
			char dst[NET_IPV6_ADDR_LEN];
			char src[NET_IPV6_ADDR_LEN];

			prepare_register_debug_print(dst, sizeof(dst),
						     src, sizeof(src),
						     remote_addr,
						     local_addr);

			NET_DBG("[%d/%d/%u/0x%02x] remote %p/%s/%u "
				"local %p/%s/%u cb %p ud %p",
				i, local_addr->family, proto, rank,
				remote_addr, dst, remote_port,
				local_addr, src, local_port,
				cb, user_data);
		} while (0);
#endif /* CONFIG_NET_DEBUG_CONN */

		if (handle) {
			*handle = (struct net_conn_handle *)&conns[i];
		}

		return 0;
	}

	return -ENOENT;
}

static bool check_addr(struct net_buf *buf,
		       struct sockaddr *addr,
		       bool is_remote)
{
	if (addr->family != net_nbuf_family(buf)) {
		return false;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6 && addr->family == AF_INET6) {
		struct in6_addr *addr6;

		if (is_remote) {
			addr6 = &NET_IPV6_BUF(buf)->src;
		} else {
			addr6 = &NET_IPV6_BUF(buf)->dst;
		}

		if (!net_is_ipv6_addr_unspecified(
			    &net_sin6(addr)->sin6_addr)) {
			if (!net_ipv6_addr_cmp(&net_sin6(addr)->sin6_addr,
					       addr6)) {
				return false;
			}
		}

		return true;
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET && addr->family == AF_INET) {
		struct in_addr *addr4;

		if (is_remote) {
			addr4 = &NET_IPV4_BUF(buf)->src;
		} else {
			addr4 = &NET_IPV4_BUF(buf)->dst;
		}

		if (net_sin(addr)->sin_addr.s_addr[0]) {
			if (!net_ipv4_addr_cmp(&net_sin(addr)->sin_addr,
					       addr4)) {
				return false;
			}
		}
	}
#endif /* CONFIG_NET_IPV4 */

	return true;
}

static inline void send_icmp_error(struct net_buf *buf)
{
	if (net_nbuf_family(buf) == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		net_icmpv6_send_error(buf, NET_ICMPV6_DST_UNREACH,
				      NET_ICMPV6_DST_UNREACH_NO_PORT, 0);
#endif /* CONFIG_NET_IPV6 */

	} else {

#if defined(CONFIG_NET_IPV4)
		net_icmpv4_send_error(buf, NET_ICMPV4_DST_UNREACH,
				      NET_ICMPV4_DST_UNREACH_NO_PORT);
#endif /* CONFIG_NET_IPV4 */
	}
}

enum net_verdict net_conn_input(enum net_ip_protocol proto, struct net_buf *buf)
{
	int i, best_match = -1;
	int16_t best_rank = -1;

#if defined(CONFIG_NET_CONN_CACHE)
	enum net_verdict verdict;
	uint32_t cache_value = 0;
	int32_t pos;

	verdict = cache_check(proto, buf, &cache_value, &pos);
	if (verdict != NET_CONTINUE) {
		return verdict;
	}
#endif

	NET_DBG("Check %s listener for buf %p src port %u dst port %u "
		"family %d", proto2str(proto), buf,
		ntohs(NET_CONN_BUF(buf)->src_port),
		ntohs(NET_CONN_BUF(buf)->dst_port),
		net_nbuf_family(buf));

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (!(conns[i].flags & NET_CONN_IN_USE)) {
			continue;
		}

		if (conns[i].proto != proto) {
			continue;
		}

		if (net_sin(&conns[i].remote_addr)->sin_port) {
			if (net_sin(&conns[i].remote_addr)->sin_port !=
			    NET_CONN_BUF(buf)->src_port) {
				continue;
			}
		}

		if (net_sin(&conns[i].local_addr)->sin_port) {
			if (net_sin(&conns[i].local_addr)->sin_port !=
			    NET_CONN_BUF(buf)->dst_port) {
				continue;
			}
		}

		if (conns[i].flags & NET_CONN_REMOTE_ADDR_SET) {
			if (!check_addr(buf, &conns[i].remote_addr, true)) {
				continue;
			}
		}

		if (conns[i].flags & NET_CONN_LOCAL_ADDR_SET) {
			if (!check_addr(buf, &conns[i].local_addr, false)) {
				continue;
			}
		}

		/* If we have an existing best_match, and that one
		 * specifies a remote port, then we've matched to a
		 * LISTENING connection that should not override.
		 */
		if (best_match >= 0 &&
		    net_sin(&conns[best_match].remote_addr)->sin_port) {
			continue;
		}

		if (best_rank < conns[i].rank) {
			best_rank = conns[i].rank;
			best_match = i;
		}
	}

	if (best_match >= 0) {
#if defined(CONFIG_NET_CONN_CACHE)
		NET_DBG("[%d] match found cb %p ud %p rank 0x%02x cache 0x%x",
			best_match,
			conns[best_match].cb,
			conns[best_match].user_data,
			conns[best_match].rank,
			pos < 0 ? 0 : conn_cache[pos].value);

		if (pos >= 0) {
			conn_cache[pos].idx = best_match;
		}
#else
		NET_DBG("[%d] match found cb %p ud %p rank 0x%02x",
			best_match,
			conns[best_match].cb,
			conns[best_match].user_data,
			conns[best_match].rank);
#endif /* CONFIG_NET_CONN_CACHE */

		if (conns[best_match].cb(&conns[best_match], buf,
			     conns[best_match].user_data) == NET_DROP) {
			goto drop;
		}

		if (proto == IPPROTO_UDP) {
			net_stats_update_udp_recv();
		}

		return NET_OK;
	}

	NET_DBG("No match found.");

	cache_add_neg(cache_value);

#if defined(CONFIG_NET_IPV6)
	/* If the destination address is multicast address,
	 * we do not send ICMP error as that makes no sense.
	 */
	if (net_nbuf_family(buf) == AF_INET6 &&
	    net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
		;
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET &&
	    net_is_ipv4_addr_mcast(&NET_IPV4_BUF(buf)->dst)) {
		;
	} else
#endif
	{
		send_icmp_error(buf);
	}

drop:
	if (proto == IPPROTO_UDP) {
		net_stats_update_udp_drop();
	}

	return NET_DROP;
}

void net_conn_init(void)
{
#if defined(CONFIG_NET_CONN_CACHE)
	do {
		int i;

		for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
			conn_cache[i].idx = -1;
		}
	} while (0);
#endif /* CONFIG_NET_CONN_CACHE */
}
