/** @file
 * @brief Generic connection related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_conn, CONFIG_NET_CONN_LOG_LEVEL);

#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/udp.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/socket_can.h>

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
				 log_strdup(net_sprint_ipv6_addr(
				    &net_sin6(&conn->remote_addr)->sin6_addr)));
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   conn->family == AF_INET) {
			snprintk(dst, sizeof(dst), "%s",
				 log_strdup(net_sprint_ipv4_addr(
				    &net_sin(&conn->remote_addr)->sin_addr)));
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
				 log_strdup(net_sprint_ipv6_addr(
				    &net_sin6(&conn->local_addr)->sin6_addr)));
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   conn->family == AF_INET) {
			snprintk(src, sizeof(src), "%s",
				 log_strdup(net_sprint_ipv4_addr(
				    &net_sin(&conn->local_addr)->sin_addr)));
		} else {
			snprintk(src, sizeof(src), "%s", "?");
		}
	} else {
		snprintk(src, sizeof(src), "%s", "-");
	}

	NET_DBG("[%p/%d/%u/0x%02x] remote %s/%u ",
		conn, conn->proto, conn->family, conn->flags,
		log_strdup(dst), remote_port);
	NET_DBG("  local %s/%u cb %p ud %p",
		log_strdup(src), local_port, conn->cb, conn->user_data);
}
#else
#define conn_register_debug(...)
#endif /* (CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG) */

static struct net_conn *conn_get_unused(void)
{
	sys_snode_t *node;

	node = sys_slist_peek_head(&conn_unused);
	if (!node) {
		return NULL;
	}

	sys_slist_remove(&conn_unused, NULL, node);

	return CONTAINER_OF(node, struct net_conn, node);
}

static void conn_set_used(struct net_conn *conn)
{
	conn->flags |= NET_CONN_IN_USE;

	sys_slist_prepend(&conn_used, &conn->node);
}

static void conn_set_unused(struct net_conn *conn)
{
	(void)memset(conn, 0, sizeof(*conn));

	sys_slist_prepend(&conn_unused, &conn->node);
}

/* Check if we already have identical connection handler installed. */
static struct net_conn *conn_find_handler(uint16_t proto, uint8_t family,
					  const struct sockaddr *remote_addr,
					  const struct sockaddr *local_addr,
					  uint16_t remote_port,
					  uint16_t local_port)
{
	struct net_conn *conn;
	struct net_conn *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn_used, conn, tmp, node) {
		if (conn->proto != proto) {
			continue;
		}

		if (conn->family != family) {
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

		if (net_sin(&conn->remote_addr)->sin_port !=
		    htons(remote_port)) {
			continue;
		}

		if (net_sin(&conn->local_addr)->sin_port !=
		    htons(local_port)) {
			continue;
		}

		return conn;
	}

	return NULL;
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

	conn = conn_find_handler(proto, family, remote_addr, local_addr,
				 remote_port, local_port);
	if (conn) {
		NET_ERR("Identical connection handler %p already found.", conn);
		return -EALREADY;
	}

	conn = conn_get_unused();
	if (!conn) {
		return -ENOENT;
	}

	if (remote_addr) {
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    remote_addr->sa_family == AF_INET6) {
			memcpy(&conn->remote_addr, remote_addr,
			       sizeof(struct sockaddr_in6));

			if (!net_ipv6_is_addr_unspecified(
				    &net_sin6(remote_addr)->
				    sin6_addr)) {
				flags |= NET_CONN_REMOTE_ADDR_SPEC;
			}
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   remote_addr->sa_family == AF_INET) {
			memcpy(&conn->remote_addr, remote_addr,
			       sizeof(struct sockaddr_in));

			if (net_sin(remote_addr)->sin_addr.s_addr) {
				flags |= NET_CONN_REMOTE_ADDR_SPEC;
			}
		} else {
			NET_ERR("Remote address family not set");
			goto error;
		}

		flags |= NET_CONN_REMOTE_ADDR_SET;
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

	if (remote_port) {
		flags |= NET_CONN_REMOTE_PORT_SPEC;
		net_sin(&conn->remote_addr)->sin_port = htons(remote_port);
	}

	if (local_port) {
		flags |= NET_CONN_LOCAL_PORT_SPEC;
		net_sin(&conn->local_addr)->sin_port = htons(local_port);
	}

	conn->cb = cb;
	conn->user_data = user_data;
	conn->flags = flags;
	conn->proto = proto;
	conn->family = family;
	conn->context = context;

	if (handle) {
		*handle = (struct net_conn_handle *)conn;
	}

	conn_set_used(conn);

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

	sys_slist_find_and_remove(&conn_used, &conn->node);

	conn_set_unused(conn);

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

static bool conn_are_end_points_valid(struct net_pkt *pkt,
				      union net_ip_header *ip_hdr,
				      uint16_t src_port,
				      uint16_t dst_port)
{
	bool my_src_addr = false;

	/* For AF_PACKET family, we are not parsing headers. */
	if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
	    net_pkt_family(pkt) == AF_PACKET) {
		return true;
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) &&
	    net_pkt_family(pkt) == AF_CAN) {
		return true;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		if (net_ipv4_addr_cmp_raw(ip_hdr->ipv4->src,
					  ip_hdr->ipv4->dst) ||
		    net_ipv4_is_my_addr((struct in_addr *)ip_hdr->ipv4->src)) {
			my_src_addr = true;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   net_pkt_family(pkt) == AF_INET6) {
		if (net_ipv6_addr_cmp_raw(ip_hdr->ipv6->src,
					  ip_hdr->ipv6->dst) ||
		    net_ipv6_is_my_addr((struct in6_addr *)ip_hdr->ipv6->src)) {
			my_src_addr = true;
		}
	}

	return !(my_src_addr && (src_port == dst_port));
}

static enum net_verdict conn_raw_socket(struct net_pkt *pkt,
					struct net_conn *conn, uint8_t proto)
{
	if (proto == ETH_P_ALL) {
		enum net_sock_type type = net_context_get_type(conn->context);

		if ((type == SOCK_DGRAM && !net_pkt_is_l2_processed(pkt)) ||
		    (type == SOCK_RAW && net_pkt_is_l2_processed(pkt))) {
			goto out;
		}
	}

	if (conn->flags & NET_CONN_LOCAL_ADDR_SET) {
		struct net_if *pkt_iface = net_pkt_iface(pkt);
		struct sockaddr_ll *local;
		struct net_pkt *raw_pkt;

		local = (struct sockaddr_ll *)&conn->local_addr;

		if (local->sll_ifindex !=
		    net_if_get_by_iface(pkt_iface)) {
			return NET_CONTINUE;
		}

		NET_DBG("[%p] raw match found cb %p ud %p", conn,
			conn->cb, conn->user_data);

		raw_pkt = net_pkt_clone(pkt, CLONE_TIMEOUT);
		if (!raw_pkt) {
			net_stats_update_per_proto_drop(pkt_iface, proto);
			NET_WARN("pkt cloning failed, pkt %p dropped", pkt);
			return NET_DROP;
		}

		if (conn->cb(conn, raw_pkt, NULL, NULL, conn->user_data)
		    == NET_DROP) {
			net_stats_update_per_proto_drop(pkt_iface, proto);
			net_pkt_unref(raw_pkt);
		} else {
			net_stats_update_per_proto_recv(pkt_iface, proto);
		}

		return NET_OK;
	}

out:
	return NET_CONTINUE;
}

enum net_verdict net_conn_input(struct net_pkt *pkt,
				union net_ip_header *ip_hdr,
				uint8_t proto,
				union net_proto_header *proto_hdr)
{
	struct net_if *pkt_iface = net_pkt_iface(pkt);
	struct net_conn *best_match = NULL;
	bool is_mcast_pkt = false, mcast_pkt_delivered = false;
	bool is_bcast_pkt = false;
	bool raw_pkt_delivered = false;
	bool raw_pkt_continue = false;
	int16_t best_rank = -1;
	struct net_conn *conn;
	enum net_verdict ret;
	uint16_t src_port;
	uint16_t dst_port;

	if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
		src_port = proto_hdr->udp->src_port;
		dst_port = proto_hdr->udp->dst_port;
	} else if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
		if (proto_hdr->tcp == NULL) {
			return NET_DROP;
		}

		src_port = proto_hdr->tcp->src_port;
		dst_port = proto_hdr->tcp->dst_port;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) {
		if (net_pkt_family(pkt) != AF_PACKET ||
		    (!IS_ENABLED(CONFIG_NET_SOCKETS_PACKET_DGRAM) &&
		     proto != ETH_P_ALL && proto != IPPROTO_RAW)) {
			return NET_DROP;
		}

		src_port = dst_port = 0U;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) &&
		   net_pkt_family(pkt) == AF_CAN) {
		if (proto != CAN_RAW) {
			return NET_DROP;
		}

		src_port = dst_port = 0U;
	} else {
		NET_DBG("No suitable protocol handler configured");
		return NET_DROP;
	}

	if (!conn_are_end_points_valid(pkt, ip_hdr, src_port, dst_port)) {
		NET_DBG("Dropping invalid src/dst end-points packet");
		return NET_DROP;
	}

	/* TODO: Make core part of networking subsystem less dependent on
	 * UDP, TCP, IPv4 or IPv6. So that we can add new features with
	 * less cross-module changes.
	 */
	NET_DBG("Check %s listener for pkt %p src port %u dst port %u"
		" family %d", net_proto2str(net_pkt_family(pkt), proto), pkt,
		ntohs(src_port), ntohs(dst_port), net_pkt_family(pkt));

	/* If we receive a packet with multicast destination address, we might
	 * need to deliver the packet to multiple recipients.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		if (net_ipv4_is_addr_mcast((struct in_addr *)ip_hdr->ipv4->dst)) {
			is_mcast_pkt = true;
		} else if (net_if_ipv4_is_addr_bcast(
				pkt_iface, (struct in_addr *)ip_hdr->ipv4->dst)) {
			is_bcast_pkt = true;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
					   net_pkt_family(pkt) == AF_INET6) {
		if (net_ipv6_is_addr_mcast((struct in6_addr *)ip_hdr->ipv6->dst)) {
			is_mcast_pkt = true;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_used, conn, node) {
		if (conn->context != NULL &&
		    net_context_is_bound_to_iface(conn->context) &&
		    net_pkt_iface(pkt) != net_context_get_iface(conn->context)) {
			continue;
		}

		/* For packet socket data, the proto is set to ETH_P_ALL or IPPROTO_RAW
		 * but the listener might have a specific protocol set. This is ok
		 * and let the packet pass this check in this case.
		 */
		if ((IS_ENABLED(CONFIG_NET_SOCKETS_PACKET_DGRAM) ||
		     IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) &&
		    net_pkt_family(pkt) == AF_PACKET) {
			if ((conn->proto != proto) && (proto != ETH_P_ALL) &&
				(proto != IPPROTO_RAW)) {
				continue;
			}
		} else {
			if ((conn->proto != proto)) {
				continue;
			}
		}

		if (conn->family != AF_UNSPEC &&
		    conn->family != net_pkt_family(pkt)) {
			/* If there are other listening connections than
			 * AF_PACKET, the packet shall be also passed back to
			 * net_conn_input() in IPv4/6 processing in order to
			 * re-check if there is any listening socket interested
			 * in this packet.
			 */
			if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
			    conn->family != AF_PACKET) {
				raw_pkt_continue = true;
			}

			continue;
		}

		/* The code below shall be only executed when one enters
		 * the net_conn_input() from net_packet_socket() which
		 * is executed for e.g. AF_PACKET && SOCK_RAW
		 *
		 * Here we do need to check if we have ANY connection which
		 * was setup with AF_PACKET
		 */
		if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
		    conn->family == AF_PACKET) {
			if (proto == ETH_P_ALL) {
				/* We shall continue with ETH_P_ALL to IPPROTO_RAW: */
				raw_pkt_continue = true;
			}

			/* With IPPROTO_RAW deliver only if protocol match: */
			if ((proto == ETH_P_ALL && conn->proto != IPPROTO_RAW) ||
			    conn->proto == proto) {
				ret = conn_raw_socket(pkt, conn, proto);
				if (ret == NET_DROP) {
					goto drop;
				} else if (ret == NET_OK) {
					raw_pkt_delivered = true;
				}

				continue;
			}
		}

		if (IS_ENABLED(CONFIG_NET_UDP) ||
		    IS_ENABLED(CONFIG_NET_TCP)) {
			if (net_sin(&conn->remote_addr)->sin_port) {
				if (net_sin(&conn->remote_addr)->sin_port !=
				    src_port) {
					continue;
				}
			}

			if (net_sin(&conn->local_addr)->sin_port) {
				if (net_sin(&conn->local_addr)->sin_port !=
				    dst_port) {
					continue;
				}
			}

			if (conn->flags & NET_CONN_REMOTE_ADDR_SET) {
				if (!conn_addr_cmp(pkt, ip_hdr,
						   &conn->remote_addr,
						   true)) {
					continue;
				}
			}

			if (conn->flags & NET_CONN_LOCAL_ADDR_SET) {
				if (!conn_addr_cmp(pkt, ip_hdr,
						   &conn->local_addr,
						   false)) {
					continue;
				}
			}

			/* If we have an existing best_match, and that one
			 * specifies a remote port, then we've matched to a
			 * LISTENING connection that should not override.
			 */
			if (best_match != NULL &&
			    best_match->flags & NET_CONN_REMOTE_PORT_SPEC) {
				continue;
			}

			if (best_rank < NET_CONN_RANK(conn->flags)) {
				struct net_pkt *mcast_pkt;

				if (!is_mcast_pkt) {
					best_rank = NET_CONN_RANK(conn->flags);
					best_match = conn;

					continue;
				}

				/* If we have a multicast packet, and we found
				 * a match, then deliver the packet immediately
				 * to the handler. As there might be several
				 * sockets interested about these, we need to
				 * clone the received pkt.
				 */

				NET_DBG("[%p] mcast match found cb %p ud %p",
					conn, conn->cb,	conn->user_data);

				mcast_pkt = net_pkt_clone(pkt, CLONE_TIMEOUT);
				if (!mcast_pkt) {
					goto drop;
				}

				if (conn->cb(conn, mcast_pkt, ip_hdr,
					     proto_hdr, conn->user_data) ==
								NET_DROP) {
					net_stats_update_per_proto_drop(
							pkt_iface, proto);
					net_pkt_unref(mcast_pkt);
				} else {
					net_stats_update_per_proto_recv(
						pkt_iface, proto);
				}

				mcast_pkt_delivered = true;
			}
		} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN)) {
			best_rank = 0;
			best_match = conn;
		}
	}

	if ((is_mcast_pkt && mcast_pkt_delivered) ||
	    (net_pkt_family(pkt) == AF_PACKET && (raw_pkt_delivered ||
						  raw_pkt_continue))) {
		if (raw_pkt_continue) {
			/* When there is open connection different than
			 * AF_PACKET this packet shall be also handled in
			 * the upper net stack layers.
			 */
			return NET_CONTINUE;
		} else {
			/* As one or more multicast or raw socket packets
			 * have already been delivered in the loop above,
			 * we shall not call the callback again here.
			 */
			net_pkt_unref(pkt);

			return NET_OK;
		}
	}

	conn = best_match;
	if (conn) {
		NET_DBG("[%p] match found cb %p ud %p rank 0x%02x",
			conn, conn->cb, conn->user_data, conn->flags);

		if (conn->cb(conn, pkt, ip_hdr, proto_hdr,
			     conn->user_data) == NET_DROP) {
			goto drop;
		}

		net_stats_update_per_proto_recv(pkt_iface, proto);

		return NET_OK;
	}

	NET_DBG("No match found.");

	/* Do not send ICMP error for Packet socket as that makes no
	 * sense here.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_pkt_family(pkt) == AF_INET6 && is_mcast_pkt) {
		;
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   net_pkt_family(pkt) == AF_INET &&
		   (is_mcast_pkt || is_bcast_pkt)) {
		;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
		    net_pkt_family(pkt) == AF_PACKET) {
		;
	} else {
		conn_send_icmp_error(pkt);

		if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
			net_stats_update_tcp_seg_connrst(pkt_iface);
		}
	}

drop:
	net_stats_update_per_proto_drop(pkt_iface, proto);

	return NET_DROP;
}

void net_conn_foreach(net_conn_foreach_cb_t cb, void *user_data)
{
	struct net_conn *conn;

	SYS_SLIST_FOR_EACH_CONTAINER(&conn_used, conn, node) {
		cb(conn, user_data);
	}
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
