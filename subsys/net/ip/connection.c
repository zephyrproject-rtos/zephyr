/** @file
 * @brief Generic connection related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_conn, CONFIG_NET_CONN_LOG_LEVEL);

#include <errno.h>
#include <misc/util.h>

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/udp.h>
#include <net/ethernet.h>
#include <net/socket_can.h>

#include "net_private.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "udp_internal.h"
#include "tcp_internal.h"
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
	u32_t value;
	s32_t idx;
};

struct conn_hash_neg {
	u32_t value;
};

/** Connection cache */
static struct conn_hash conn_cache[CONFIG_NET_MAX_CONN];

/** Negative cache, we definitely do not have a connection
 * to these hosts.
 */
static struct conn_hash_neg conn_cache_neg[CONFIG_NET_MAX_CONN];

#define TAKE_BIT(val, bit, max, used)				\
	(((val & BIT(bit)) >> bit) << (max - used))

static inline u8_t ports_to_hash(u16_t remote_port,
				    u16_t local_port)
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

static inline u16_t ipv6_to_hash(struct in6_addr *addr)
{
	/* There is 11 bits available for IPv6 address */
	/* Use more bits from the lower part of address space */
	return
		/* Take 3 bits from higher values */
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[0]), 31, 11, 1) |
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[0]), 15, 11, 2) |
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[0]), 7, 11, 3) |

		/* Take 2 bits from higher middle values */
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[1]), 31, 11, 4) |
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[1]), 15, 11, 5) |

		/* Take 2 bits from lower middle values */
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[2]), 31, 11, 6) |
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[2]), 15, 11, 7) |

		/* Take 4 bits from lower values */
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[3]), 31, 11, 8) |
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[3]), 15, 11, 9) |
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[3]), 7, 11, 10) |
		TAKE_BIT(UNALIGNED_GET(&addr->s6_addr32[3]), 0, 11, 11);
}

static inline u16_t ipv4_to_hash(struct in_addr *addr)
{
	/* There is 11 bits available for IPv4 address */
	/* Use more bits from the lower part of address space */
	return
		TAKE_BIT(addr->s_addr, 31, 11, 1) |
		TAKE_BIT(addr->s_addr, 27, 11, 2) |
		TAKE_BIT(addr->s_addr, 21, 11, 3) |
		TAKE_BIT(addr->s_addr, 17, 11, 4) |
		TAKE_BIT(addr->s_addr, 14, 11, 5) |
		TAKE_BIT(addr->s_addr, 11, 11, 6) |
		TAKE_BIT(addr->s_addr, 8, 11, 7) |
		TAKE_BIT(addr->s_addr, 5, 11, 8) |
		TAKE_BIT(addr->s_addr, 3, 11, 9) |
		TAKE_BIT(addr->s_addr, 2, 11, 10) |
		TAKE_BIT(addr->s_addr, 0, 11, 11);
}

/* Return either the first free position in the cache (idx < 0) or
 * the existing cached position (idx >= 0)
 */
static s32_t check_hash(enum net_ip_protocol proto,
			  sa_family_t family,
			  void *remote_addr,
			  void *local_addr,
			  u16_t remote_port,
			  u16_t local_port,
			  u32_t *cache_value)
{
	int i, free_pos = -1;
	u32_t value = 0U;

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

static inline s32_t get_conn(struct net_pkt *pkt,
			     union net_ip_header *ip_hdr,
			     enum net_ip_protocol proto,
			     u16_t src_port,
			     u16_t dst_port,
			     u32_t *cache_value)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		return check_hash(proto, net_pkt_family(pkt),
				  &ip_hdr->ipv4->src, &ip_hdr->ipv4->dst,
				  src_port, dst_port, cache_value);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		return check_hash(proto, net_pkt_family(pkt),
				  &ip_hdr->ipv6->src, &ip_hdr->ipv6->dst,
				  src_port, dst_port, cache_value);
	}

	return -1;
}

static inline void cache_add_neg(u32_t cache_value)
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

static inline bool cache_check_neg(u32_t cache_value)
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
		conn_cache_neg[i].value = 0U;
	}
}

static inline enum net_verdict cache_check(struct net_pkt *pkt,
					   union net_ip_header *ip_hdr,
					   union net_proto_header *proto_hdr,
					   enum net_ip_protocol proto,
					   u16_t src_port,
					   u16_t dst_port,
					   u32_t *cache_value,
					   s32_t *pos)
{
	*pos = get_conn(pkt, ip_hdr, proto, src_port, dst_port, cache_value);
	if (*pos >= 0) {
		if (conn_cache[*pos].idx >= 0) {
			/* Connection is in the cache */
			struct net_conn *conn = &conns[conn_cache[*pos].idx];

			NET_DBG("Cache %s listener for pkt %p src port %u "
				"dst port %u family %d cache[%d] 0x%x",
				net_proto2str(net_pkt_family(pkt), proto), pkt,
				src_port, dst_port,
				net_pkt_family(pkt), *pos,
				conn_cache[*pos].value);

			return conn->cb(conn, pkt,
					ip_hdr, proto_hdr, conn->user_data);
		}
	} else if (*cache_value > 0) {
		if (cache_check_neg(*cache_value)) {
			NET_DBG("Drop by cache");
			return NET_DROP;
		}
	}

	return NET_CONTINUE;
}

static inline void cache_remove(struct net_conn *conn)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (conn_cache[i].idx < 0 ||
		    conn_cache[i].idx >= CONFIG_NET_MAX_CONN) {
			continue;
		}

		if (&conns[conn_cache[i].idx] == conn) {
			conn_cache[i].idx = -1;
			break;
		}
	}
}
#else
#define cache_clear(...)
#define cache_add_neg(...)
#define cache_check(...) NET_CONTINUE
#define cache_remove(...)
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

	cache_remove(conn);

	NET_DBG("[%zu] connection handler %p removed",
		conn - conns, conn);

	(void)memset(conn, 0, sizeof(*conn));

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
		conn - conns, conn);

	conn->cb = cb;
	conn->user_data = user_data;

	return 0;
}

static inline
void prepare_register_debug_print(char *dst, int dst_len,
				  char *src, int src_len,
				  const struct sockaddr *remote_addr,
				  const struct sockaddr *local_addr)
{
	if (remote_addr && remote_addr->sa_family == AF_INET6) {
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			snprintk(dst, dst_len, "%s",
				 log_strdup(net_sprint_ipv6_addr(
					  &net_sin6(remote_addr)->sin6_addr)));
		} else {
			snprintk(dst, dst_len, "%s", "?");
		}

	} else if (remote_addr && remote_addr->sa_family == AF_INET) {
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			snprintk(dst, dst_len, "%s",
				 log_strdup(net_sprint_ipv4_addr(
					  &net_sin(remote_addr)->sin_addr)));
		} else {
			snprintk(dst, dst_len, "%s", "?");
		}

	} else {
		snprintk(dst, dst_len, "%s", "-");
	}

	if (local_addr && local_addr->sa_family == AF_INET6) {
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			snprintk(src, src_len, "%s",
				 log_strdup(net_sprint_ipv6_addr(
					  &net_sin6(local_addr)->sin6_addr)));
		} else {
			snprintk(src, src_len, "%s", "?");
		}

	} else if (local_addr && local_addr->sa_family == AF_INET) {
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			snprintk(src, src_len, "%s",
				 log_strdup(net_sprint_ipv4_addr(
					  &net_sin(local_addr)->sin_addr)));
		} else {
			snprintk(src, src_len, "%s", "?");
		}

	} else {
		snprintk(src, src_len, "%s", "-");
	}
}

/* Check if we already have identical connection handler installed. */
static int find_conn_handler(u16_t proto, u8_t family,
			     const struct sockaddr *remote_addr,
			     const struct sockaddr *local_addr,
			     u16_t remote_port,
			     u16_t local_port)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (!(conns[i].flags & NET_CONN_IN_USE)) {
			continue;
		}

		if (conns[i].proto != proto) {
			continue;
		}

		if (conns[i].family != family) {
			continue;
		}

		if (remote_addr) {
			if (!(conns[i].flags & NET_CONN_REMOTE_ADDR_SET)) {
				continue;
			}

#if defined(CONFIG_NET_IPV6)
			if (remote_addr->sa_family == AF_INET6 &&
			    remote_addr->sa_family ==
			    conns[i].remote_addr.sa_family) {
				if (!net_ipv6_addr_cmp(
					    &net_sin6(remote_addr)->sin6_addr,
					    &net_sin6(&conns[i].remote_addr)->
								sin6_addr)) {
					continue;
				}
			} else
#endif
#if defined(CONFIG_NET_IPV4)
			if (remote_addr->sa_family == AF_INET &&
			    remote_addr->sa_family ==
			    conns[i].remote_addr.sa_family) {
				if (!net_ipv4_addr_cmp(
					    &net_sin(remote_addr)->sin_addr,
					    &net_sin(&conns[i].remote_addr)->
								sin_addr)) {
					continue;
				}
			} else
#endif
			{
				continue;
			}
		} else {
			if (conns[i].flags & NET_CONN_REMOTE_ADDR_SET) {
				continue;
			}
		}

		if (local_addr) {
			if (!(conns[i].flags & NET_CONN_LOCAL_ADDR_SET)) {
				continue;
			}

#if defined(CONFIG_NET_IPV6)
			if (local_addr->sa_family == AF_INET6 &&
			    local_addr->sa_family ==
			    conns[i].local_addr.sa_family) {
				if (!net_ipv6_addr_cmp(
					    &net_sin6(local_addr)->sin6_addr,
					    &net_sin6(&conns[i].local_addr)->
								sin6_addr)) {
					continue;
				}
			} else
#endif
#if defined(CONFIG_NET_IPV4)
			if (local_addr->sa_family == AF_INET &&
			    local_addr->sa_family ==
			    conns[i].local_addr.sa_family) {
				if (!net_ipv4_addr_cmp(
					    &net_sin(local_addr)->sin_addr,
					    &net_sin(&conns[i].local_addr)->
								sin_addr)) {
					continue;
				}
			} else
#endif
			{
				continue;
			}
		} else {
			if (conns[i].flags & NET_CONN_LOCAL_ADDR_SET) {
				continue;
			}
		}

		if (net_sin(&conns[i].remote_addr)->sin_port !=
		    htons(remote_port)) {
			continue;
		}

		if (net_sin(&conns[i].local_addr)->sin_port !=
		    htons(local_port)) {
			continue;
		}

		return i;
	}

	return -ENOENT;
}

int net_conn_register(u16_t proto, u8_t family,
		      const struct sockaddr *remote_addr,
		      const struct sockaddr *local_addr,
		      u16_t remote_port,
		      u16_t local_port,
		      net_conn_cb_t cb,
		      void *user_data,
		      struct net_conn_handle **handle)
{
	int i;
	u8_t rank = 0U;

	i = find_conn_handler(proto, family, remote_addr, local_addr,
			      remote_port, local_port);
	if (i != -ENOENT) {
		NET_ERR("Identical connection handler %p already found.",
			&conns[i]);
		return -EALREADY;
	}

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (conns[i].flags & NET_CONN_IN_USE) {
			continue;
		}

		if (remote_addr) {
#if defined(CONFIG_NET_IPV6)
			if (remote_addr->sa_family == AF_INET6) {
				memcpy(&conns[i].remote_addr, remote_addr,
				       sizeof(struct sockaddr_in6));

				if (net_ipv6_is_addr_unspecified(
					    &net_sin6(remote_addr)->
							sin6_addr)) {
					rank |= NET_RANK_REMOTE_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_REMOTE_SPEC_ADDR;
				}
			} else
#endif

#if defined(CONFIG_NET_IPV4)
			if (remote_addr->sa_family == AF_INET) {
				memcpy(&conns[i].remote_addr, remote_addr,
				       sizeof(struct sockaddr_in));

				if (!net_sin(remote_addr)->
							sin_addr.s_addr) {
					rank |= NET_RANK_REMOTE_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_REMOTE_SPEC_ADDR;
				}
			} else
#endif
			{
				NET_ERR("Remote address family not set");
				return -EINVAL;
			}

			conns[i].flags |= NET_CONN_REMOTE_ADDR_SET;
		}

		if (local_addr) {
#if defined(CONFIG_NET_IPV6)
			if (local_addr->sa_family == AF_INET6) {
				memcpy(&conns[i].local_addr, local_addr,
				       sizeof(struct sockaddr_in6));

				if (net_ipv6_is_addr_unspecified(
					    &net_sin6(local_addr)->
							sin6_addr)) {
					rank |= NET_RANK_LOCAL_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_LOCAL_SPEC_ADDR;
				}
			} else
#endif

#if defined(CONFIG_NET_IPV4)
			if (local_addr->sa_family == AF_INET) {
				memcpy(&conns[i].local_addr, local_addr,
				       sizeof(struct sockaddr_in));

				if (!net_sin(local_addr)->sin_addr.s_addr) {
					rank |= NET_RANK_LOCAL_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_LOCAL_SPEC_ADDR;
				}
			} else
#endif
			{
				NET_ERR("Local address family not set");
				return -EINVAL;
			}

			conns[i].flags |= NET_CONN_LOCAL_ADDR_SET;
		}

		if (remote_addr && local_addr) {
			if (remote_addr->sa_family != local_addr->sa_family) {
				NET_ERR("Address families different");
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
		conns[i].family = family;

		/* Cache needs to be cleared if new entries are added. */
		cache_clear();

		if (CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG) {
			char dst[NET_IPV6_ADDR_LEN];
			char src[NET_IPV6_ADDR_LEN];

			prepare_register_debug_print(dst, sizeof(dst),
						     src, sizeof(src),
						     remote_addr,
						     local_addr);

			NET_DBG("[%d/%d/%u/%u/0x%02x] remote %p/%s/%u ", i,
				local_addr ? local_addr->sa_family : AF_UNSPEC,
				proto, family, rank, remote_addr,
				log_strdup(dst), remote_port);
			NET_DBG("  local %p/%s/%u cb %p ud %p",
				local_addr, log_strdup(src), local_port,
				cb, user_data);
		}

		if (handle) {
			*handle = (struct net_conn_handle *)&conns[i];
		}

		return 0;
	}

	return -ENOENT;
}

static bool check_addr(struct net_pkt *pkt,
		       union net_ip_header *ip_hdr,
		       struct sockaddr *addr,
		       bool is_remote)
{
	if (addr->sa_family != net_pkt_family(pkt)) {
		return false;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6 && addr->sa_family == AF_INET6) {
		struct in6_addr *addr6;

		if (is_remote) {
			addr6 = &ip_hdr->ipv6->src;
		} else {
			addr6 = &ip_hdr->ipv6->dst;
		}

		if (!net_ipv6_is_addr_unspecified(
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
	if (net_pkt_family(pkt) == AF_INET && addr->sa_family == AF_INET) {
		struct in_addr *addr4;

		if (is_remote) {
			addr4 = &ip_hdr->ipv4->src;
		} else {
			addr4 = &ip_hdr->ipv4->dst;
		}

		if (net_sin(addr)->sin_addr.s_addr) {
			if (!net_ipv4_addr_cmp(&net_sin(addr)->sin_addr,
					       addr4)) {
				return false;
			}
		}
	}
#endif /* CONFIG_NET_IPV4 */

	return true;
}

static inline void send_icmp_error(struct net_pkt *pkt)
{
	if (net_pkt_family(pkt) == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		net_icmpv6_send_error(pkt, NET_ICMPV6_DST_UNREACH,
				      NET_ICMPV6_DST_UNREACH_NO_PORT, 0);
#endif /* CONFIG_NET_IPV6 */

	} else {

#if defined(CONFIG_NET_IPV4)
		net_icmpv4_send_error(pkt, NET_ICMPV4_DST_UNREACH,
				      NET_ICMPV4_DST_UNREACH_NO_PORT);
#endif /* CONFIG_NET_IPV4 */
	}
}

static bool is_invalid_packet(struct net_pkt *pkt,
			      union net_ip_header *ip_hdr,
			      u16_t src_port,
			      u16_t dst_port)
{
	if (src_port != dst_port) {
		return false;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		if (net_ipv4_addr_cmp(&ip_hdr->ipv4->src, &ip_hdr->ipv4->dst) ||
		    net_ipv4_is_my_addr(&ip_hdr->ipv4->src)) {
			return true;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		if (net_ipv6_addr_cmp(&ip_hdr->ipv6->src, &ip_hdr->ipv6->dst) ||
		    net_ipv6_is_my_addr(&ip_hdr->ipv6->src)) {
			return true;
		}
	}

	/* For AF_PACKET family, we are not not parsing headers. */
	if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
	    net_pkt_family(pkt) == AF_PACKET) {
		return false;
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) &&
	    net_pkt_family(pkt) == AF_CAN) {
		return false;
	}

	return true;
}

enum net_verdict net_conn_input(struct net_pkt *pkt,
				union net_ip_header *ip_hdr,
				u8_t proto,
				union net_proto_header *proto_hdr)
{
	struct net_if *pkt_iface = net_pkt_iface(pkt);
	int i, best_match = -1;
	s16_t best_rank = -1;
	u16_t src_port;
	u16_t dst_port;
#if defined(CONFIG_NET_CONN_CACHE)
	enum net_verdict verdict;
	u32_t cache_value = 0U;
	s32_t pos;
#endif

	if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
		src_port = proto_hdr->udp->src_port;
		dst_port = proto_hdr->udp->dst_port;
	} else if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
		src_port = proto_hdr->tcp->src_port;
		dst_port = proto_hdr->tcp->dst_port;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) {
		if (net_pkt_family(pkt) != AF_PACKET || proto != ETH_P_ALL) {
			return NET_DROP;
		}

		src_port = dst_port = 0;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) &&
		   net_pkt_family(pkt) == AF_CAN) {
		if (proto != CAN_RAW) {
			return NET_DROP;
		}

		src_port = dst_port = 0;
	} else {
		NET_DBG("No suitable protocol handler configured");
		return NET_DROP;
	}

	if (is_invalid_packet(pkt, ip_hdr, src_port, dst_port)) {
		NET_DBG("Dropping invalid packet");
		return NET_DROP;
	}

	/* TODO : Make core part of networing subsystem less depedant on
	 * UDP, TCP, IPv4 or IPv6. So that we can add new features with
	 * less dependency.
	 */
#if defined(CONFIG_NET_CONN_CACHE) && \
	(defined(CONFIG_NET_UDP) || defined(CONFIG_NET_TCP))
	verdict = cache_check(pkt, ip_hdr, proto_hdr, proto, src_port, dst_port,
			      &cache_value, &pos);
	if (verdict != NET_CONTINUE) {
		return verdict;
	}
#endif

	NET_DBG("Check %s listener for pkt %p src port %u dst port %u"
		" family %d", net_proto2str(net_pkt_family(pkt), proto), pkt,
		ntohs(src_port), ntohs(dst_port), net_pkt_family(pkt));

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (!(conns[i].flags & NET_CONN_IN_USE)) {
			continue;
		}

		if (conns[i].proto != proto) {
			continue;
		}

		if (conns[i].family != AF_UNSPEC &&
		    conns[i].family != net_pkt_family(pkt)) {
			continue;
		}

		if (IS_ENABLED(CONFIG_NET_UDP) ||
		    IS_ENABLED(CONFIG_NET_TCP)) {
			if (net_sin(&conns[i].remote_addr)->sin_port) {
				if (net_sin(&conns[i].remote_addr)->sin_port !=
				    src_port) {
					continue;
				}
			}

			if (net_sin(&conns[i].local_addr)->sin_port) {
				if (net_sin(&conns[i].local_addr)->sin_port !=
				    dst_port) {
					continue;
				}
			}

			if (conns[i].flags & NET_CONN_REMOTE_ADDR_SET) {
				if (!check_addr(pkt, ip_hdr,
						&conns[i].remote_addr,
						true)) {
					continue;
				}
			}

			if (conns[i].flags & NET_CONN_LOCAL_ADDR_SET) {
				if (!check_addr(pkt, ip_hdr,
						&conns[i].local_addr,
						false)) {
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
		} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) {
			best_rank = 0;
			best_match = i;
		} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN)) {
			best_rank = 0;
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

		if (conns[best_match].cb(&conns[best_match], pkt, ip_hdr,
			proto_hdr, conns[best_match].user_data) == NET_DROP) {
			goto drop;
		}

		net_stats_update_per_proto_recv(pkt_iface, proto);

		return NET_OK;
	}

	NET_DBG("No match found.");

	cache_add_neg(cache_value);

	/* If the destination address is multicast address,
	 * we will not send an ICMP error as that makes no sense.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_pkt_family(pkt) == AF_INET6 &&
	    net_ipv6_is_addr_mcast(&ip_hdr->ipv6->dst)) {
		;
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   net_pkt_family(pkt) == AF_INET &&
		   net_ipv4_is_addr_mcast(&ip_hdr->ipv4->dst)) {
		;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
		    net_pkt_family(pkt) == AF_PACKET) {
		;
	} else {
		send_icmp_error(pkt);

		if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
			net_stats_update_tcp_seg_connrst(net_pkt_iface(pkt));
		}
	}

drop:
	net_stats_update_per_proto_drop(pkt_iface, proto);

	return NET_DROP;
}

void net_conn_foreach(net_conn_foreach_cb_t cb, void *user_data)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (!(conns[i].flags & NET_CONN_IN_USE)) {
			continue;
		}

		cb(&conns[i], user_data);
	}
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
