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
#include <net/net_pkt.h>

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
#define NET_CONN_HDR(pkt) ((struct net_udp_hdr *)(net_pkt_udp_data(pkt)))

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
	u32_t value = 0;

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

static inline s32_t get_conn(enum net_ip_protocol proto,
			       sa_family_t family,
			       struct net_pkt *pkt,
			       u32_t *cache_value)
{
#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		return check_hash(proto, family,
				  &NET_IPV4_HDR(pkt)->src,
				  &NET_IPV4_HDR(pkt)->dst,
				  NET_UDP_HDR(pkt)->src_port,
				  NET_UDP_HDR(pkt)->dst_port,
				  cache_value);
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		return check_hash(proto, family,
				  &NET_IPV6_HDR(pkt)->src,
				  &NET_IPV6_HDR(pkt)->dst,
				  NET_UDP_HDR(pkt)->src_port,
				  NET_UDP_HDR(pkt)->dst_port,
				  cache_value);
	}
#endif

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
		conn_cache_neg[i].value = 0;
	}
}

static inline enum net_verdict cache_check(enum net_ip_protocol proto,
					   struct net_pkt *pkt,
					   u32_t *cache_value,
					   s32_t *pos)
{
	*pos = get_conn(proto, net_pkt_family(pkt), pkt, cache_value);
	if (*pos >= 0) {
		if (conn_cache[*pos].idx >= 0) {
			/* Connection is in the cache */
			struct net_conn *conn;

			conn = &conns[conn_cache[*pos].idx];

			NET_DBG("Cache %s listener for pkt %p src port %u "
				"dst port %u family %d cache[%d] 0x%x",
				net_proto2str(proto), pkt,
				ntohs(NET_CONN_HDR(pkt)->src_port),
				ntohs(NET_CONN_HDR(pkt)->dst_port),
				net_pkt_family(pkt), *pos,
				conn_cache[*pos].value);

			return conn->cb(conn, pkt, conn->user_data);
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

/* Check if we already have identical connection handler installed. */
static int find_conn_handler(enum net_ip_protocol proto,
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

		if (remote_addr) {
			if (!(conns[i].flags & NET_CONN_REMOTE_ADDR_SET)) {
				continue;
			}

#if defined(CONFIG_NET_IPV6)
			if (remote_addr->family == AF_INET6 &&
			    remote_addr->family ==
			    conns[i].remote_addr.family) {
				if (!net_ipv6_addr_cmp(
					    &net_sin6(remote_addr)->sin6_addr,
					    &net_sin6(&conns[i].remote_addr)->
								sin6_addr)) {
					continue;
				}
			} else
#endif
#if defined(CONFIG_NET_IPV4)
			if (remote_addr->family == AF_INET &&
			    remote_addr->family ==
			    conns[i].remote_addr.family) {
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
			if (local_addr->family == AF_INET6 &&
			    local_addr->family ==
			    conns[i].local_addr.family) {
				if (!net_ipv6_addr_cmp(
					    &net_sin6(local_addr)->sin6_addr,
					    &net_sin6(&conns[i].local_addr)->
								sin6_addr)) {
					continue;
				}
			} else
#endif
#if defined(CONFIG_NET_IPV4)
			if (local_addr->family == AF_INET &&
			    local_addr->family ==
			    conns[i].local_addr.family) {
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

int net_conn_register(enum net_ip_protocol proto,
		      const struct sockaddr *remote_addr,
		      const struct sockaddr *local_addr,
		      u16_t remote_port,
		      u16_t local_port,
		      net_conn_cb_t cb,
		      void *user_data,
		      struct net_conn_handle **handle)
{
	int i;
	u8_t rank = 0;

	i = find_conn_handler(proto, remote_addr, local_addr, remote_port,
			      local_port);
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
			if (remote_addr->family != AF_INET &&
			    remote_addr->family != AF_INET6) {
				NET_ERR("Remote address family not set.");
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
							sin_addr.s_addr) {
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
				NET_ERR("Local address family not set.");
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
				if (!net_sin(local_addr)->sin_addr.s_addr) {
					rank |= NET_RANK_LOCAL_UNSPEC_ADDR;
				} else {
					rank |= NET_RANK_LOCAL_SPEC_ADDR;
				}
			}
#endif
		}

		if (remote_addr && local_addr) {
			if (remote_addr->family != local_addr->family) {
				NET_ERR("Address families different.");
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

static bool check_addr(struct net_pkt *pkt,
		       struct sockaddr *addr,
		       bool is_remote)
{
	if (addr->family != net_pkt_family(pkt)) {
		return false;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6 && addr->family == AF_INET6) {
		struct in6_addr *addr6;

		if (is_remote) {
			addr6 = &NET_IPV6_HDR(pkt)->src;
		} else {
			addr6 = &NET_IPV6_HDR(pkt)->dst;
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
	if (net_pkt_family(pkt) == AF_INET && addr->family == AF_INET) {
		struct in_addr *addr4;

		if (is_remote) {
			addr4 = &NET_IPV4_HDR(pkt)->src;
		} else {
			addr4 = &NET_IPV4_HDR(pkt)->dst;
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

enum net_verdict net_conn_input(enum net_ip_protocol proto, struct net_pkt *pkt)
{
	int i, best_match = -1;
	s16_t best_rank = -1;
	u16_t chksum;

#if defined(CONFIG_NET_CONN_CACHE)
	enum net_verdict verdict;
	u32_t cache_value = 0;
	s32_t pos;

	verdict = cache_check(proto, pkt, &cache_value, &pos);
	if (verdict != NET_CONTINUE) {
		return verdict;
	}
#endif

	if (proto == IPPROTO_TCP) {
		chksum = NET_TCP_HDR(pkt)->chksum;
	} else {
		chksum = NET_UDP_HDR(pkt)->chksum;
	}

	if (IS_ENABLED(CONFIG_NET_DEBUG_CONN)) {
		NET_DBG("Check %s listener for pkt %p src port %u dst port %u "
			"family %d chksum 0x%04x", net_proto2str(proto), pkt,
			ntohs(NET_CONN_HDR(pkt)->src_port),
			ntohs(NET_CONN_HDR(pkt)->dst_port),
			net_pkt_family(pkt), ntohs(chksum));
	}

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		if (!(conns[i].flags & NET_CONN_IN_USE)) {
			continue;
		}

		if (conns[i].proto != proto) {
			continue;
		}

		if (net_sin(&conns[i].remote_addr)->sin_port) {
			if (net_sin(&conns[i].remote_addr)->sin_port !=
			    NET_CONN_HDR(pkt)->src_port) {
				continue;
			}
		}

		if (net_sin(&conns[i].local_addr)->sin_port) {
			if (net_sin(&conns[i].local_addr)->sin_port !=
			    NET_CONN_HDR(pkt)->dst_port) {
				continue;
			}
		}

		if (conns[i].flags & NET_CONN_REMOTE_ADDR_SET) {
			if (!check_addr(pkt, &conns[i].remote_addr, true)) {
				continue;
			}
		}

		if (conns[i].flags & NET_CONN_LOCAL_ADDR_SET) {
			if (!check_addr(pkt, &conns[i].local_addr, false)) {
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

		/* If packet has a listener configured, then check also the
		 * protocol checksum if that checking is enabled.
		 * If the checksum calculation fails, then discard the message.
		 */
		if (IS_ENABLED(CONFIG_NET_UDP_CHECKSUM) &&
		    proto == IPPROTO_UDP) {
			u16_t chksum_calc;

			NET_UDP_HDR(pkt)->chksum = 0;
			chksum_calc = ~net_calc_chksum_udp(pkt);

			if (chksum != chksum_calc) {
				net_stats_update_udp_chkerr();
				goto drop;
			}

			NET_UDP_HDR(pkt)->chksum = chksum;

		} else if (IS_ENABLED(CONFIG_NET_TCP_CHECKSUM) &&
			   proto == IPPROTO_TCP) {
			u16_t chksum_calc;

			NET_TCP_HDR(pkt)->chksum = 0;
			chksum_calc = ~net_calc_chksum_tcp(pkt);

			if (chksum != chksum_calc) {
				net_stats_update_tcp_seg_chkerr();
				goto drop;
			}

			NET_TCP_HDR(pkt)->chksum = chksum;
		}

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

		if (conns[best_match].cb(&conns[best_match], pkt,
			     conns[best_match].user_data) == NET_DROP) {
			goto drop;
		}

		net_stats_update_per_proto_recv(proto);

		return NET_OK;
	}

	NET_DBG("No match found.");

	cache_add_neg(cache_value);

#if defined(CONFIG_NET_IPV6)
	/* If the destination address is multicast address,
	 * we do not send ICMP error as that makes no sense.
	 */
	if (net_pkt_family(pkt) == AF_INET6 &&
	    net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst)) {
		;
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET &&
	    net_is_ipv4_addr_mcast(&NET_IPV4_HDR(pkt)->dst)) {
		;
	} else
#endif
	{
		send_icmp_error(pkt);

		if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
			net_stats_update_tcp_seg_connrst();
		}
	}

drop:
	net_stats_update_per_proto_drop(proto);

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
