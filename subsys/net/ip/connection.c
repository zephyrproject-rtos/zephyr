/** @file
 * @brief Generic connection related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TODO: Make core part of networking subsystem less dependent on
 * UDP, TCP, IPv4 or IPv6. So that we can add new features with
 * less cross-module changes.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_conn, CONFIG_NET_CONN_LOG_LEVEL);

#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/udp.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/socketcan.h>

#include "net_private.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "connection.h"
#include "net_stats.h"

/** How long to wait for when cloning multicast packet */
#define CLONE_TIMEOUT K_MSEC(100)

/** Is this connection used or not */
#define NET_CONN_IN_USE			BIT(0)

/** Remote address set */
#define NET_CONN_REMOTE_ADDR_SET	BIT(1)

/** Local address set */
#define NET_CONN_LOCAL_ADDR_SET		BIT(2)

/** Local port set */
#define NET_CONN_REMOTE_PORT_SPEC	BIT(3)

/** Remote port set */
#define NET_CONN_LOCAL_PORT_SPEC	BIT(4)

/** Local address specified */
#define NET_CONN_REMOTE_ADDR_SPEC	BIT(5)

/** Remote address specified */
#define NET_CONN_LOCAL_ADDR_SPEC	BIT(6)

#define NET_CONN_RANK(_flags)		(_flags & 0x78)

static struct net_conn conns[CONFIG_NET_MAX_CONN];

static sys_slist_t conn_unused;
static sys_slist_t conn_used;

#if (CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG)
static inline
void conn_register_debug(struct net_conn *conn,
			 uint16_t remote_port, uint16_t local_port)
{
	char dst[NET_IPV6_ADDR_LEN];
	char src[NET_IPV6_ADDR_LEN];

	if (conn->flags & NET_CONN_REMOTE_ADDR_SET) {
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    conn->family == AF_INET6) {
			snprintk(dst, sizeof(dst), "%s",
				 net_sprint_ipv6_addr(&net_sin6(&conn->remote_addr)->sin6_addr));
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   conn->family == AF_INET) {
			snprintk(dst, sizeof(dst), "%s",
				 net_sprint_ipv4_addr(&net_sin(&conn->remote_addr)->sin_addr));
		} else {
			snprintk(dst, sizeof(dst), "%s", "?");
		}
	} else {
		snprintk(dst, sizeof(dst), "%s", "-");
	}

	if (conn->flags & NET_CONN_LOCAL_ADDR_SET) {
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    conn->family == AF_INET6) {
			snprintk(src, sizeof(src), "%s",
				 net_sprint_ipv6_addr(&net_sin6(&conn->local_addr)->sin6_addr));
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   conn->family == AF_INET) {
			snprintk(src, sizeof(src), "%s",
				 net_sprint_ipv4_addr(&net_sin(&conn->local_addr)->sin_addr));
		} else {
			snprintk(src, sizeof(src), "%s", "?");
		}
	} else {
		snprintk(src, sizeof(src), "%s", "-");
	}

	NET_DBG("[%p/%d/%u/0x%02x] remote %s/%u ",
		conn, conn->proto, conn->family, conn->flags,
		dst, remote_port);
	NET_DBG("  local %s/%u cb %p ud %p",
		src, local_port, conn->cb, conn->user_data);
}
#else
#define conn_register_debug(...)
#endif /* (CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG) */

static K_MUTEX_DEFINE(conn_lock);

static struct net_conn *conn_get_unused(void)
{
	sys_snode_t *node;

	k_mutex_lock(&conn_lock, K_FOREVER);

	node = sys_slist_peek_head(&conn_unused);
	if (!node) {
		k_mutex_unlock(&conn_lock);
		return NULL;
	}

	sys_slist_remove(&conn_unused, NULL, node);

	k_mutex_unlock(&conn_lock);

	return CONTAINER_OF(node, struct net_conn, node);
}

static void conn_set_used(struct net_conn *conn)
{
	conn->flags |= NET_CONN_IN_USE;

	k_mutex_lock(&conn_lock, K_FOREVER);
	sys_slist_prepend(&conn_used, &conn->node);
	k_mutex_unlock(&conn_lock);
}

static void conn_set_unused(struct net_conn *conn)
{
	(void)memset(conn, 0, sizeof(*conn));

	k_mutex_lock(&conn_lock, K_FOREVER);
	sys_slist_prepend(&conn_unused, &conn->node);
	k_mutex_unlock(&conn_lock);
}

/* Check if we already have identical connection handler installed. */
static struct net_conn *conn_find_handler(struct net_if *iface,
					  uint16_t proto, uint8_t family,
					  const struct sockaddr *remote_addr,
					  const struct sockaddr *local_addr,
					  uint16_t remote_port,
					  uint16_t local_port,
					  bool reuseport_set)
{
	struct net_conn *conn;
	struct net_conn *tmp;

	k_mutex_lock(&conn_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn_used, conn, tmp, node) {
		if (conn->proto != proto) {
			continue;
		}

		if (conn->family != family) {
			continue;
		}

		if (local_addr) {
			if (!(conn->flags & NET_CONN_LOCAL_ADDR_SET)) {
				continue;
			}

			if (IS_ENABLED(CONFIG_NET_IPV6) &&
			    local_addr->sa_family == AF_INET6 &&
			    local_addr->sa_family ==
			    conn->local_addr.sa_family) {
				if (!net_ipv6_addr_cmp(
					    &net_sin6(local_addr)->sin6_addr,
					    &net_sin6(&conn->local_addr)->
								sin6_addr)) {
					continue;
				}
			} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
				   local_addr->sa_family == AF_INET &&
				   local_addr->sa_family ==
				   conn->local_addr.sa_family) {
				if (!net_ipv4_addr_cmp(
					    &net_sin(local_addr)->sin_addr,
					    &net_sin(&conn->local_addr)->
								sin_addr)) {
					continue;
				}
			} else {
				continue;
			}
		} else if (conn->flags & NET_CONN_LOCAL_ADDR_SET) {
			continue;
		}

		if (net_sin(&conn->local_addr)->sin_port !=
		    htons(local_port)) {
			continue;
		}

		if (remote_addr) {
			if (!(conn->flags & NET_CONN_REMOTE_ADDR_SET)) {
				continue;
			}

			if (IS_ENABLED(CONFIG_NET_IPV6) &&
			    remote_addr->sa_family == AF_INET6 &&
			    remote_addr->sa_family ==
			    conn->remote_addr.sa_family) {
				if (!net_ipv6_addr_cmp(
					    &net_sin6(remote_addr)->sin6_addr,
					    &net_sin6(&conn->remote_addr)->
								sin6_addr)) {
					continue;
				}
			} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
				   remote_addr->sa_family == AF_INET &&
				   remote_addr->sa_family ==
				   conn->remote_addr.sa_family) {
				if (!net_ipv4_addr_cmp(
					    &net_sin(remote_addr)->sin_addr,
					    &net_sin(&conn->remote_addr)->
								sin_addr)) {
					continue;
				}
			} else {
				continue;
			}
		} else if (conn->flags & NET_CONN_REMOTE_ADDR_SET) {
			continue;
		} else if (reuseport_set && conn->context != NULL &&
			   net_context_is_reuseport_set(conn->context)) {
			continue;
		}

		if (net_sin(&conn->remote_addr)->sin_port !=
		    htons(remote_port)) {
			continue;
		}

		if (conn->context != NULL && iface != NULL &&
		    net_context_is_bound_to_iface(conn->context)) {
			if (iface != net_context_get_iface(conn->context)) {
				continue;
			}
		}

		k_mutex_unlock(&conn_lock);
		return conn;
	}

	k_mutex_unlock(&conn_lock);
	return NULL;
}

static void net_conn_change_callback(struct net_conn *conn,
				     net_conn_cb_t cb, void *user_data)
{
	NET_DBG("[%zu] connection handler %p changed callback",
		conn - conns, conn);

	conn->cb = cb;
	conn->user_data = user_data;
}

static int net_conn_change_remote(struct net_conn *conn,
				  const struct sockaddr *remote_addr,
				  uint16_t remote_port)
{
	NET_DBG("[%zu] connection handler %p changed remote",
		conn - conns, conn);

	if (remote_addr) {
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    remote_addr->sa_family == AF_INET6) {
			memcpy(&conn->remote_addr, remote_addr,
			       sizeof(struct sockaddr_in6));

			if (!net_ipv6_is_addr_unspecified(
				    &net_sin6(remote_addr)->
				    sin6_addr)) {
				conn->flags |= NET_CONN_REMOTE_ADDR_SPEC;
			}
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   remote_addr->sa_family == AF_INET) {
			memcpy(&conn->remote_addr, remote_addr,
			       sizeof(struct sockaddr_in));

			if (net_sin(remote_addr)->sin_addr.s_addr) {
				conn->flags |= NET_CONN_REMOTE_ADDR_SPEC;
			}
		} else {
			NET_ERR("Remote address family not set");
			return -EINVAL;
		}

		conn->flags |= NET_CONN_REMOTE_ADDR_SET;
	} else {
		conn->flags &= ~NET_CONN_REMOTE_ADDR_SPEC;
		conn->flags &= ~NET_CONN_REMOTE_ADDR_SET;
	}

	if (remote_port) {
		conn->flags |= NET_CONN_REMOTE_PORT_SPEC;
		net_sin(&conn->remote_addr)->sin_port = htons(remote_port);
	} else {
		conn->flags &= ~NET_CONN_REMOTE_PORT_SPEC;
	}

	return 0;
}

int net_conn_register(uint16_t proto, uint8_t family,
		      const struct sockaddr *remote_addr,
		      const struct sockaddr *local_addr,
		      uint16_t remote_port,
		      uint16_t local_port,
		      struct net_context *context,
		      net_conn_cb_t cb,
		      void *user_data,
		      struct net_conn_handle **handle)
{
	struct net_conn *conn;
	uint8_t flags = 0U;
	int ret;

	conn = conn_find_handler(context != NULL ? net_context_get_iface(context) : NULL,
				 proto, family, remote_addr, local_addr,
				 remote_port, local_port,
				 context != NULL ?
					net_context_is_reuseport_set(context) :
					false);
	if (conn) {
		NET_ERR("Identical connection handler %p already found.", conn);
		return -EADDRINUSE;
	}

	conn = conn_get_unused();
	if (!conn) {
		NET_ERR("Not enough connection contexts. "
			"Consider increasing CONFIG_NET_MAX_CONN.");
		return -ENOENT;
	}

	if (local_addr) {
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    local_addr->sa_family == AF_INET6) {
			memcpy(&conn->local_addr, local_addr,
			       sizeof(struct sockaddr_in6));

			if (!net_ipv6_is_addr_unspecified(
				    &net_sin6(local_addr)->
				    sin6_addr)) {
				flags |= NET_CONN_LOCAL_ADDR_SPEC;
			}
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   local_addr->sa_family == AF_INET) {
			memcpy(&conn->local_addr, local_addr,
			       sizeof(struct sockaddr_in));

			if (net_sin(local_addr)->sin_addr.s_addr) {
				flags |= NET_CONN_LOCAL_ADDR_SPEC;
			}
		} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) &&
			   local_addr->sa_family == AF_CAN) {
			memcpy(&conn->local_addr, local_addr,
			       sizeof(struct sockaddr_can));
		} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
			   local_addr->sa_family == AF_PACKET) {
			memcpy(&conn->local_addr, local_addr,
			       sizeof(struct sockaddr_ll));
		} else {
			NET_ERR("Local address family not set");
			goto error;
		}

		flags |= NET_CONN_LOCAL_ADDR_SET;
	}

	if (remote_addr && local_addr) {
		if (remote_addr->sa_family != local_addr->sa_family) {
			NET_ERR("Address families different");
			goto error;
		}
	}

	if (local_port) {
		flags |= NET_CONN_LOCAL_PORT_SPEC;
		net_sin(&conn->local_addr)->sin_port = htons(local_port);
	}

	net_conn_change_callback(conn, cb, user_data);

	conn->flags = flags;
	conn->proto = proto;
	conn->family = family;
	conn->context = context;

	/*
	 * Since the net_conn_change_remote() updates the flags in connection,
	 * must to be called after set the flags to connection.
	 */
	ret = net_conn_change_remote(conn, remote_addr, remote_port);
	if (ret) {
		goto error;
	}

	if (handle) {
		*handle = (struct net_conn_handle *)conn;
	}

	conn_set_used(conn);

	conn->v6only = net_context_is_v6only_set(context);

	conn_register_debug(conn, remote_port, local_port);

	return 0;
error:
	conn_set_unused(conn);
	return -EINVAL;
}

int net_conn_unregister(struct net_conn_handle *handle)
{
	struct net_conn *conn = (struct net_conn *)handle;

	if (conn < &conns[0] || conn > &conns[CONFIG_NET_MAX_CONN]) {
		return -EINVAL;
	}

	if (!(conn->flags & NET_CONN_IN_USE)) {
		return -ENOENT;
	}

	NET_DBG("Connection handler %p removed", conn);

	k_mutex_lock(&conn_lock, K_FOREVER);
	sys_slist_find_and_remove(&conn_used, &conn->node);
	k_mutex_unlock(&conn_lock);

	conn_set_unused(conn);

	return 0;
}

int net_conn_update(struct net_conn_handle *handle,
		    net_conn_cb_t cb,
		    void *user_data,
		    const struct sockaddr *remote_addr,
		    uint16_t remote_port)
{
	struct net_conn *conn = (struct net_conn *)handle;
	int ret;

	if (conn < &conns[0] || conn > &conns[CONFIG_NET_MAX_CONN]) {
		return -EINVAL;
	}

	if (!(conn->flags & NET_CONN_IN_USE)) {
		return -ENOENT;
	}

	net_conn_change_callback(conn, cb, user_data);

	ret = net_conn_change_remote(conn, remote_addr, remote_port);

	return ret;
}

static bool conn_addr_cmp(struct net_pkt *pkt,
			  union net_ip_header *ip_hdr,
			  struct sockaddr *addr,
			  bool is_remote)
{
	if (addr->sa_family != net_pkt_family(pkt)) {
		return false;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_pkt_family(pkt) == AF_INET6 &&
	    addr->sa_family == AF_INET6) {
		uint8_t *addr6;

		if (is_remote) {
			addr6 = ip_hdr->ipv6->src;
		} else {
			addr6 = ip_hdr->ipv6->dst;
		}

		if (!net_ipv6_is_addr_unspecified(
			    &net_sin6(addr)->sin6_addr)) {
			if (!net_ipv6_addr_cmp_raw((uint8_t *)&net_sin6(addr)->sin6_addr,
						   addr6)) {
				return false;
			}
		}

		return true;
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   net_pkt_family(pkt) == AF_INET &&
		   addr->sa_family == AF_INET) {
		uint8_t *addr4;

		if (is_remote) {
			addr4 = ip_hdr->ipv4->src;
		} else {
			addr4 = ip_hdr->ipv4->dst;
		}

		if (net_sin(addr)->sin_addr.s_addr) {
			if (!net_ipv4_addr_cmp_raw((uint8_t *)&net_sin(addr)->sin_addr,
						   addr4)) {
				return false;
			}
		}
	}

	return true;
}

static inline void conn_send_icmp_error(struct net_pkt *pkt)
{
	if (IS_ENABLED(CONFIG_NET_DISABLE_ICMP_DESTINATION_UNREACHABLE)) {
		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		net_icmpv6_send_error(pkt, NET_ICMPV6_DST_UNREACH,
				      NET_ICMPV6_DST_UNREACH_NO_PORT, 0);

	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_icmpv4_send_error(pkt, NET_ICMPV4_DST_UNREACH,
				      NET_ICMPV4_DST_UNREACH_NO_PORT);
	}
}

static bool conn_are_endpoints_valid(struct net_pkt *pkt, uint8_t family,
				     union net_ip_header *ip_hdr,
				     uint16_t src_port, uint16_t dst_port)
{
	bool is_my_src_addr;
	bool is_same_src_and_dst_addr;

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		is_my_src_addr = net_ipv4_is_my_addr(
			(struct in_addr *)ip_hdr->ipv4->src);
		is_same_src_and_dst_addr = net_ipv4_addr_cmp_raw(
			ip_hdr->ipv4->src, ip_hdr->ipv4->dst);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		is_my_src_addr = net_ipv6_is_my_addr(
			(struct in6_addr *)ip_hdr->ipv6->src);
		is_same_src_and_dst_addr = net_ipv6_addr_cmp_raw(
			ip_hdr->ipv6->src, ip_hdr->ipv6->dst);
	} else {
		return true;
	}

	bool is_same_src_and_dst_port = src_port == dst_port;
	bool are_invalid_endpoints =
		(is_same_src_and_dst_addr || is_my_src_addr) && is_same_src_and_dst_port;

	return !are_invalid_endpoints;
}

static enum net_verdict conn_raw_socket(struct net_pkt *pkt,
					struct net_conn *conn, uint8_t proto)
{
	enum net_sock_type type = net_context_get_type(conn->context);

	if (proto == ETH_P_ALL) {
		if ((type == SOCK_DGRAM && !net_pkt_is_l2_processed(pkt)) ||
		    (type == SOCK_RAW && net_pkt_is_l2_processed(pkt))) {
			return NET_CONTINUE;
		}
	}

	/*
	 * After l2 processed only deliver protocol matched pkt,
	 * unless the connection protocol is all packets
	 */
	if (type == SOCK_DGRAM && net_pkt_is_l2_processed(pkt) &&
	    conn->proto != ETH_P_ALL &&
	    conn->proto != net_pkt_ll_proto_type(pkt)) {
		return NET_CONTINUE;
	}

	if (!(conn->flags & NET_CONN_LOCAL_ADDR_SET)) {
		return NET_CONTINUE;
	}

	struct net_if *pkt_iface = net_pkt_iface(pkt);
	struct sockaddr_ll *local;
	struct net_pkt *raw_pkt;

	local = (struct sockaddr_ll *)&conn->local_addr;

	if (local->sll_ifindex != net_if_get_by_iface(pkt_iface)) {
		return NET_CONTINUE;
	}

	NET_DBG("[%p] raw match found cb %p ud %p", conn, conn->cb,
		conn->user_data);

	raw_pkt = net_pkt_clone(pkt, CLONE_TIMEOUT);
	if (!raw_pkt) {
		net_stats_update_per_proto_drop(pkt_iface, proto);
		NET_WARN("pkt cloning failed, pkt %p dropped", pkt);
		return NET_DROP;
	}

	if (conn->cb(conn, raw_pkt, NULL, NULL, conn->user_data) == NET_DROP) {
		net_stats_update_per_proto_drop(pkt_iface, proto);
		net_pkt_unref(raw_pkt);
	} else {
		net_stats_update_per_proto_recv(pkt_iface, proto);
	}

	return NET_OK;
}

enum net_verdict net_conn_input(struct net_pkt *pkt,
				union net_ip_header *ip_hdr,
				uint8_t proto,
				union net_proto_header *proto_hdr)
{
	struct net_if *pkt_iface = net_pkt_iface(pkt);
	uint8_t pkt_family = net_pkt_family(pkt);
	uint16_t src_port = 0U, dst_port = 0U;

	if (!net_pkt_filter_local_in_recv_ok(pkt)) {
		/* drop the packet */
		return NET_DROP;
	}

	if (IS_ENABLED(CONFIG_NET_IP) && (pkt_family == AF_INET || pkt_family == AF_INET6)) {
		if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
			src_port = proto_hdr->udp->src_port;
			dst_port = proto_hdr->udp->dst_port;
		} else if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
			if (proto_hdr->tcp == NULL) {
				return NET_DROP;
			}
			src_port = proto_hdr->tcp->src_port;
			dst_port = proto_hdr->tcp->dst_port;
		}
		if (!conn_are_endpoints_valid(pkt, pkt_family, ip_hdr, src_port, dst_port)) {
			NET_DBG("Dropping invalid src/dst end-points packet");
			return NET_DROP;
		}
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) && pkt_family == AF_PACKET) {
		if (proto != ETH_P_ALL && proto != IPPROTO_RAW) {
			return NET_DROP;
		}
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) && pkt_family == AF_CAN) {
		if (proto != CAN_RAW) {
			return NET_DROP;
		}
	} else {
		NET_DBG("No suitable protocol handler configured");
		return NET_DROP;
	}

	NET_DBG("Check %s listener for pkt %p src port %u dst port %u"
		" family %d", net_proto2str(net_pkt_family(pkt), proto), pkt,
		ntohs(src_port), ntohs(dst_port), net_pkt_family(pkt));


	struct net_conn *best_match = NULL;
	int16_t best_rank = -1;
	bool is_mcast_pkt = false;
	bool mcast_pkt_delivered = false;
	bool is_bcast_pkt = false;
	bool raw_pkt_delivered = false;
	bool raw_pkt_continue = false;
	struct net_conn *conn;
	net_conn_cb_t cb = NULL;
	void *user_data = NULL;

	if (IS_ENABLED(CONFIG_NET_IP)) {
		/* If we receive a packet with multicast destination address, we might
		 * need to deliver the packet to multiple recipients.
		 */
		if (IS_ENABLED(CONFIG_NET_IPV4) && pkt_family == AF_INET) {
			if (net_ipv4_is_addr_mcast((struct in_addr *)ip_hdr->ipv4->dst)) {
				is_mcast_pkt = true;
			} else if (net_if_ipv4_is_addr_bcast(pkt_iface,
							     (struct in_addr *)ip_hdr->ipv4->dst)) {
				is_bcast_pkt = true;
			}
		} else if (IS_ENABLED(CONFIG_NET_IPV6) && pkt_family == AF_INET6) {
			is_mcast_pkt = net_ipv6_is_addr_mcast((struct in6_addr *)ip_hdr->ipv6->dst);
		}
	}

	k_mutex_lock(&conn_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_used, conn, node) {
		/* Is the candidate connection matching the packet's interface? */
		if (conn->context != NULL &&
		    net_context_is_bound_to_iface(conn->context) &&
		    net_pkt_iface(pkt) != net_context_get_iface(conn->context)) {
			continue; /* wrong interface */
		}

		/* Is the candidate connection matching the packet's protocol family? */
		if (conn->family != AF_UNSPEC &&
		    conn->family != pkt_family) {
			if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) {
				/* If there are other listening connections than
				 * AF_PACKET, the packet shall be also passed back to
				 * net_conn_input() in upper layer processing in order to
				 * re-check if there is any listening socket interested
				 * in this packet.
				 */
				if (conn->family != AF_PACKET) {
					raw_pkt_continue = true;
				}
			}

			if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
				if (!(conn->family == AF_INET6 && pkt_family == AF_INET &&
				      !conn->v6only)) {
					continue;
				}
			} else {
				continue; /* wrong protocol family */
			}

			/* We might have a match for v4-to-v6 mapping, check more */
		}

		/* Is the candidate connection matching the packet's protocol within the family? */
		if (conn->proto != proto) {
			/* For packet socket data, the proto is set to ETH_P_ALL
			 * or IPPROTO_RAW but the listener might have a specific
			 * protocol set. This is ok and let the packet pass this
			 * check in this case.
			 */
			if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) && pkt_family == AF_PACKET) {
				if (proto != ETH_P_ALL && proto != IPPROTO_RAW) {
					continue; /* wrong protocol */
				}
			} else {
				continue; /* wrong protocol */
			}
		}

		/* Apply protocol-specific matching criteria... */
		uint8_t conn_family = conn->family;

		if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) && conn_family == AF_PACKET) {
			/* This code shall be only executed when one enters
			 * the net_conn_input() from net_packet_socket() which
			 * targets AF_PACKET sockets.
			 *
			 * All AF_PACKET connections will receive the packet if
			 * their socket type and - in case of IPPROTO - protocol
			 * also matches.
			 */
			if (proto == ETH_P_ALL) {
				/* We shall continue with ETH_P_ALL to IPPROTO_RAW: */
				raw_pkt_continue = true;
			}

			/* With IPPROTO_RAW deliver only if protocol match: */
			if ((proto == ETH_P_ALL && conn->proto != IPPROTO_RAW) ||
			    conn->proto == proto) {
				enum net_verdict ret = conn_raw_socket(pkt, conn, proto);

				if (ret == NET_DROP) {
					k_mutex_unlock(&conn_lock);
					goto drop;
				} else if (ret == NET_OK) {
					raw_pkt_delivered = true;
				}

				continue; /* packet was consumed */
			}
		} else if ((IS_ENABLED(CONFIG_NET_UDP) || IS_ENABLED(CONFIG_NET_TCP)) &&
			   (conn_family == AF_INET || conn_family == AF_INET6 ||
			    conn_family == AF_UNSPEC)) {
			/* Is the candidate connection matching the packet's TCP/UDP
			 * address and port?
			 */
			if (net_sin(&conn->remote_addr)->sin_port &&
			    net_sin(&conn->remote_addr)->sin_port != src_port) {
				continue; /* wrong remote port */
			}

			if (net_sin(&conn->local_addr)->sin_port &&
			    net_sin(&conn->local_addr)->sin_port != dst_port) {
				continue; /* wrong local port */
			}

			if ((conn->flags & NET_CONN_REMOTE_ADDR_SET) &&
			    !conn_addr_cmp(pkt, ip_hdr, &conn->remote_addr, true)) {
				continue; /* wrong remote address */
			}

			if ((conn->flags & NET_CONN_LOCAL_ADDR_SET) &&
			    !conn_addr_cmp(pkt, ip_hdr, &conn->local_addr, false)) {

				/* Check if we could do a v4-mapping-to-v6 and the IPv6 socket
				 * has no IPV6_V6ONLY option set and if the local IPV6 address
				 * is unspecified, then we could accept a connection from IPv4
				 * address by mapping it to IPv6 address.
				 */
				if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
					if (!(conn->family == AF_INET6 && pkt_family == AF_INET &&
					      !conn->v6only &&
					      net_ipv6_is_addr_unspecified(
						      &net_sin6(&conn->local_addr)->sin6_addr))) {
						continue; /* wrong local address */
					}
				} else {
					continue; /* wrong local address */
				}

				/* We might have a match for v4-to-v6 mapping,
				 * continue with rank checking.
				 */
			}

			if (best_rank < NET_CONN_RANK(conn->flags)) {
				struct net_pkt *mcast_pkt;

				if (!is_mcast_pkt) {
					best_rank = NET_CONN_RANK(conn->flags);
					best_match = conn;

					continue; /* found a match - but maybe not yet the best */
				}

				/* If we have a multicast packet, and we found
				 * a match, then deliver the packet immediately
				 * to the handler. As there might be several
				 * sockets interested about these, we need to
				 * clone the received pkt.
				 */

				NET_DBG("[%p] mcast match found cb %p ud %p", conn, conn->cb,
					conn->user_data);

				mcast_pkt = net_pkt_clone(pkt, CLONE_TIMEOUT);
				if (!mcast_pkt) {
					k_mutex_unlock(&conn_lock);
					goto drop;
				}

				if (conn->cb(conn, mcast_pkt, ip_hdr, proto_hdr, conn->user_data) ==
				    NET_DROP) {
					net_stats_update_per_proto_drop(pkt_iface, proto);
					net_pkt_unref(mcast_pkt);
				} else {
					net_stats_update_per_proto_recv(pkt_iface, proto);
				}

				mcast_pkt_delivered = true;
			}
		} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) && conn_family == AF_CAN) {
			best_match = conn;
		}
	} /* loop end */

	if (best_match) {
		cb = best_match->cb;
		user_data = best_match->user_data;
	}

	k_mutex_unlock(&conn_lock);

	if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) && pkt_family == AF_PACKET) {
		if (raw_pkt_continue) {
			/* When there is open connection different than
			 * AF_PACKET this packet shall be also handled in
			 * the upper net stack layers.
			 */
			return NET_CONTINUE;
		}
		if (raw_pkt_delivered) {
			/* As one or more raw socket packets
			 * have already been delivered in the loop above,
			 * we shall not call the callback again here.
			 */
			net_pkt_unref(pkt);
			return NET_OK;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IP) && is_mcast_pkt && mcast_pkt_delivered) {
		/* As one or more multicast packets
		 * have already been delivered in the loop above,
		 * we shall not call the callback again here.
		 */
		net_pkt_unref(pkt);
		return NET_OK;
	}

	if (cb) {
		NET_DBG("[%p] match found cb %p ud %p rank 0x%02x", best_match, cb,
			user_data, NET_CONN_RANK(best_match->flags));

		if (cb(best_match, pkt, ip_hdr, proto_hdr, user_data)
				== NET_DROP) {
			goto drop;
		}

		net_stats_update_per_proto_recv(pkt_iface, proto);

		return NET_OK;
	}

	NET_DBG("No match found.");

	if (IS_ENABLED(CONFIG_NET_IP) && (pkt_family == AF_INET || pkt_family == AF_INET6) &&
	    !(is_mcast_pkt || is_bcast_pkt)) {
		if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP &&
		    IS_ENABLED(CONFIG_NET_TCP_REJECT_CONN_WITH_RST)) {
			net_tcp_reply_rst(pkt);
			net_stats_update_tcp_seg_connrst(pkt_iface);
		} else {
			conn_send_icmp_error(pkt);
		}
	}

drop:
	net_stats_update_per_proto_drop(pkt_iface, proto);

	return NET_DROP;
}

void net_conn_foreach(net_conn_foreach_cb_t cb, void *user_data)
{
	struct net_conn *conn;

	k_mutex_lock(&conn_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_used, conn, node) {
		cb(conn, user_data);
	}

	k_mutex_unlock(&conn_lock);
}

void net_conn_init(void)
{
	int i;

	sys_slist_init(&conn_unused);
	sys_slist_init(&conn_used);

	for (i = 0; i < CONFIG_NET_MAX_CONN; i++) {
		sys_slist_prepend(&conn_unused, &conns[i].node);
	}
}
