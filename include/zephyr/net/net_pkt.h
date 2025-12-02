/** @file
 * @brief Network packet buffer descriptor API
 *
 * Network data is passed between different parts of the stack via
 * net_buf struct.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Data buffer API - used for all data to/from net */

#ifndef ZEPHYR_INCLUDE_NET_NET_PKT_H_
#define ZEPHYR_INCLUDE_NET_NET_PKT_H_

#include <zephyr/types.h>
#include <stdbool.h>

#include <zephyr/net_buf.h>

#if defined(CONFIG_IEEE802154)
#include <zephyr/net/ieee802154_pkt.h>
#endif
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_linkaddr.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_time.h>
#include <zephyr/net/ethernet_vlan.h>
#include <zephyr/net/ptp_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network packet management library
 * @defgroup net_pkt Network Packet Library
 * @since 1.5
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

struct net_context;

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_NET_PKT_ALLOC_STATS)
struct net_pkt_alloc_stats {
	uint64_t alloc_sum;
	uint64_t time_sum;
	uint32_t count;
};

struct net_pkt_alloc_stats_slab {
	struct net_pkt_alloc_stats ok;
	struct net_pkt_alloc_stats fail;
	struct k_mem_slab *slab;
};

#define NET_PKT_ALLOC_STATS_DEFINE(alloc_name, slab_name)		  \
	STRUCT_SECTION_ITERABLE(net_pkt_alloc_stats_slab, alloc_name) = { \
		.slab = &slab_name,					  \
	}

#else
#define NET_PKT_ALLOC_STATS_DEFINE(name, slab)
#endif /* CONFIG_NET_PKT_ALLOC_STATS */

/* buffer cursor used in net_pkt */
struct net_pkt_cursor {
	/** Current net_buf pointer by the cursor */
	struct net_buf *buf;
	/** Current position in the data buffer of the net_buf */
	uint8_t *pos;
};

/** @endcond */

/**
 * @brief Network packet.
 *
 * Note that if you add new fields into net_pkt, remember to update
 * net_pkt_clone() function.
 */
struct net_pkt {
	/**
	 * The fifo is used by RX/TX threads and by socket layer. The net_pkt
	 * is queued via fifo to the processing thread.
	 */
	intptr_t fifo;

	/** Slab pointer from where it belongs to */
	struct k_mem_slab *slab;

	/** buffer holding the packet */
	union {
		struct net_buf *frags;   /**< buffer fragment */
		struct net_buf *buffer;  /**< alias to a buffer fragment */
	};

	/** Internal buffer iterator used for reading/writing */
	struct net_pkt_cursor cursor;

	/** Network connection context */
	struct net_context *context;

	/** Network interface */
	struct net_if *iface;

	/** @cond ignore */

#if defined(CONFIG_NET_TCP)
	/** Allow placing the packet into sys_slist_t */
	sys_snode_t next;
#endif
#if defined(CONFIG_NET_ROUTING) || defined(CONFIG_NET_ETHERNET_BRIDGE)
	struct net_if *orig_iface; /* Original network interface */
#endif

#if defined(CONFIG_NET_VPN)
	struct {
		/** Original network interface */
		struct net_if *iface;
		/** Pointer to IP header of the encrypted pkt */
		union net_ip_header ip_hdr;
		/** Pointer to UDP header of the encrypted pkt */
		union net_proto_header proto_hdr;
		/** Peer id */
		int peer_id;
	} vpn;
#endif

#if defined(CONFIG_NET_PKT_TIMESTAMP) || defined(CONFIG_NET_PKT_TXTIME)
	/**
	 * TX or RX timestamp if available
	 *
	 * For packets that have been sent over the medium, the timestamp refers
	 * to the time the message timestamp point was encountered at the
	 * reference plane.
	 *
	 * Unsent packages can be scheduled by setting the timestamp to a future
	 * point in time.
	 *
	 * All timestamps refer to the network subsystem's local clock.
	 *
	 * See @ref net_ptp_time for definitions of local clock, message
	 * timestamp point and reference plane. See @ref net_time_t for
	 * semantics of the network reference clock.
	 *
	 * TODO: Replace with net_time_t to decouple from PTP.
	 */
	struct net_ptp_time timestamp;
#endif

#if defined(CONFIG_NET_PKT_RXTIME_STATS) || defined(CONFIG_NET_PKT_TXTIME_STATS) || \
	defined(CONFIG_TRACING_NET_CORE)
	struct {
		/** Create time in cycles */
		uint32_t create_time;

#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL) || \
	defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
		/** Collect extra statistics for net_pkt processing
		 * from various points in the IP stack. See networking
		 * documentation where these points are located and how
		 * to interpret the results.
		 */
		struct {
			uint32_t stat[NET_PKT_DETAIL_STATS_COUNT];
			int count;
		} detail;
#endif /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL ||
	  CONFIG_NET_PKT_RXTIME_STATS_DETAIL */
	};
#endif /* CONFIG_NET_PKT_RXTIME_STATS || CONFIG_NET_PKT_TXTIME_STATS */

#if defined(CONFIG_NET_PKT_ALLOC_STATS)
	struct net_pkt_alloc_stats_slab *alloc_stats;
#endif /* CONFIG_NET_PKT_ALLOC_STATS */

	/** Reference counter */
	atomic_t atomic_ref;

	/* Filled by layer 2 when network packet is received. */
	struct net_linkaddr lladdr_src;
	struct net_linkaddr lladdr_dst;
	uint16_t ll_proto_type;

#if defined(CONFIG_NET_IP)
	uint8_t ip_hdr_len;	/* pre-filled in order to avoid func call */
#endif

	uint8_t overwrite : 1;	 /* Is packet content being overwritten? */
	uint8_t eof : 1;	 /* Last packet before EOF */
	uint8_t ptp_pkt : 1;	 /* For outgoing packet: is this packet
				  * a L2 PTP packet.
				  * Used only if defined (CONFIG_NET_L2_PTP)
				  */
	uint8_t forwarding : 1;	 /* Are we forwarding this pkt
				  * Used only if defined(CONFIG_NET_ROUTE)
				  */
	uint8_t family : 3;	 /* Address family, see net_ip.h */

	/* bitfield byte alignment boundary */

#if defined(CONFIG_NET_IPV4_ACD)
	uint8_t ipv4_acd_arp_msg : 1;  /* Is this pkt IPv4 conflict detection ARP
					* message.
					* Note: family needs to be
					* AF_INET.
					*/
#endif
#if defined(CONFIG_NET_LLDP)
	uint8_t lldp_pkt : 1; /* Is this pkt an LLDP message.
			       * Note: family needs to be
			       * AF_UNSPEC.
			       */
#endif
	uint8_t ppp_msg : 1; /* This is a PPP message */
	uint8_t captured : 1;	  /* Set to 1 if this packet is already being
				   * captured
				   */
	uint8_t l2_bridged : 1;	  /* set to 1 if this packet comes from a bridge
				   * and already contains its L2 header to be
				   * preserved. Useful only if
				   * defined(CONFIG_NET_ETHERNET_BRIDGE).
				   */
	uint8_t l2_processed : 1; /* Set to 1 if this packet has already been
				   * processed by the L2
				   */
	uint8_t chksum_done : 1; /* Checksum has already been computed for
				  * the packet.
				  */
	uint8_t loopback : 1; /* Packet is a loop back packet. */
#if defined(CONFIG_NET_IP_FRAGMENT)
	uint8_t ip_reassembled : 1; /* Packet is a reassembled IP packet. */
#endif
#if defined(CONFIG_NET_PKT_TIMESTAMP)
	uint8_t tx_timestamping : 1; /** Timestamp transmitted packet */
	uint8_t rx_timestamping : 1; /** Timestamp received packet */
#endif
	/* bitfield byte alignment boundary */

#if defined(CONFIG_NET_IP)
	union {
		/* IPv6 hop limit or IPv4 ttl for this network packet.
		 * The value is shared between IPv6 and IPv4.
		 */
#if defined(CONFIG_NET_IPV6)
		uint8_t ipv6_hop_limit;
#endif
#if defined(CONFIG_NET_IPV4)
		uint8_t ipv4_ttl;
#endif
	};

	union {
#if defined(CONFIG_NET_IPV4)
		uint8_t ipv4_opts_len; /* length of IPv4 header options */
#endif
#if defined(CONFIG_NET_IPV6)
		uint16_t ipv6_ext_len; /* length of extension headers */
#endif
	};

#if defined(CONFIG_NET_IP_FRAGMENT)
	union {
#if defined(CONFIG_NET_IPV4_FRAGMENT)
		struct {
			uint16_t flags;		/* Fragment offset and M (More Fragment) flag */
			uint16_t id;		/* Fragment ID */
		} ipv4_fragment;
#endif /* CONFIG_NET_IPV4_FRAGMENT */
#if defined(CONFIG_NET_IPV6_FRAGMENT)
		struct {
			uint16_t flags;		/* Fragment offset and M (More Fragment) flag */
			uint32_t id;		/* Fragment id */
			uint16_t hdr_start;	/* Where starts the fragment header */
		} ipv6_fragment;
#endif /* CONFIG_NET_IPV6_FRAGMENT */
	};
#endif /* CONFIG_NET_IP_FRAGMENT */

#if defined(CONFIG_NET_IPV6)
	/* Where is the start of the last header before payload data
	 * in IPv6 packet. This is offset value from start of the IPv6
	 * packet. Note that this value should be updated by who ever
	 * adds IPv6 extension headers to the network packet.
	 */
	uint16_t ipv6_prev_hdr_start;

	uint8_t ipv6_ext_opt_len; /* IPv6 ND option length */
	uint8_t ipv6_next_hdr;	/* What is the very first next header */
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IP_DSCP_ECN)
	/** IPv4/IPv6 Differentiated Services Code Point value. */
	uint8_t ip_dscp : 6;

	/** IPv4/IPv6 Explicit Congestion Notification value. */
	uint8_t ip_ecn : 2;
#endif /* CONFIG_NET_IP_DSCP_ECN */
#endif /* CONFIG_NET_IP */

#if defined(CONFIG_NET_VLAN)
	/* VLAN TCI (Tag Control Information). This contains the Priority
	 * Code Point (PCP), Drop Eligible Indicator (DEI) and VLAN
	 * Identifier (VID, called more commonly VLAN tag). This value is
	 * kept in host byte order.
	 */
	uint16_t vlan_tci;
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_NET_PKT_CONTROL_BLOCK)
	/* Control block which could be used by any layer */
	union {
		uint8_t cb[CONFIG_NET_PKT_CONTROL_BLOCK_SIZE];
#if defined(CONFIG_IEEE802154)
		/* The following structure requires a 4-byte alignment
		 * boundary to avoid padding.
		 */
		struct net_pkt_cb_ieee802154 cb_ieee802154;
#endif /* CONFIG_IEEE802154 */
	} cb;
#endif /* CONFIG_NET_PKT_CONTROL_BLOCK */

	/** Network packet priority, can be left out in which case packet
	 * is not prioritised.
	 */
	uint8_t priority;

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_L2_IPIP)
	/* Remote address of the received packet. This is only used by
	 * network interfaces with an offloaded TCP/IP stack, or if we
	 * have network tunneling in use.
	 */
	union {
		struct sockaddr remote;

		/* This will make sure that there is enough storage to store
		 * the address struct. The access to value is via remote
		 * address.
		 */
		struct sockaddr_storage remote_storage;
	};
#endif /* CONFIG_NET_OFFLOAD */

#if defined(CONFIG_NET_CAPTURE_COOKED_MODE)
	/* Tell the capture api that this is a captured packet */
	uint8_t cooked_mode_pkt : 1;
#endif /* CONFIG_NET_CAPTURE_COOKED_MODE */

#if defined(CONFIG_NET_IPV4_PMTU)
	/* Path MTU needed for this destination address */
	uint8_t ipv4_pmtu : 1;
#endif /* CONFIG_NET_IPV4_PMTU */

	/* @endcond */
};

/** @cond ignore */

/* The interface real ll address */
static inline struct net_linkaddr *net_pkt_lladdr_if(struct net_pkt *pkt)
{
	return net_if_get_link_addr(pkt->iface);
}

static inline struct net_context *net_pkt_context(struct net_pkt *pkt)
{
	return pkt->context;
}

static inline void net_pkt_set_context(struct net_pkt *pkt,
				       struct net_context *ctx)
{
	pkt->context = ctx;
}

static inline struct net_if *net_pkt_iface(struct net_pkt *pkt)
{
	return pkt->iface;
}

static inline void net_pkt_set_iface(struct net_pkt *pkt, struct net_if *iface)
{
	pkt->iface = iface;

	/* If the network interface is set in pkt, then also set the type of
	 * the network address that is stored in pkt. This is done here so
	 * that the address type is properly set and is not forgotten.
	 */
	if (iface) {
		uint8_t type = net_if_get_link_addr(iface)->type;

		pkt->lladdr_src.type = type;
		pkt->lladdr_dst.type = type;
	}
}

static inline struct net_if *net_pkt_orig_iface(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_ROUTING) || defined(CONFIG_NET_ETHERNET_BRIDGE)
	return pkt->orig_iface;
#else
	return pkt->iface;
#endif
}

static inline void net_pkt_set_orig_iface(struct net_pkt *pkt,
					  struct net_if *iface)
{
#if defined(CONFIG_NET_ROUTING) || defined(CONFIG_NET_ETHERNET_BRIDGE)
	pkt->orig_iface = iface;
#else
	ARG_UNUSED(pkt);
	ARG_UNUSED(iface);
#endif
}

#if defined(CONFIG_NET_VPN)
static inline struct net_if *net_pkt_vpn_iface(struct net_pkt *pkt)
{
	return pkt->vpn.iface;
}

static inline void net_pkt_set_vpn_iface(struct net_pkt *pkt,
					 struct net_if *iface)
{
	pkt->vpn.iface = iface;
}

static inline union net_ip_header *net_pkt_vpn_ip_hdr(struct net_pkt *pkt)
{
	return &pkt->vpn.ip_hdr;
}

static inline void net_pkt_set_vpn_ip_hdr(struct net_pkt *pkt,
					  union net_ip_header *ip_hdr)
{
	pkt->vpn.ip_hdr = *ip_hdr;
}

static inline union net_proto_header *net_pkt_vpn_udp_hdr(struct net_pkt *pkt)
{
	return &pkt->vpn.proto_hdr;
}

static inline void net_pkt_set_vpn_udp_hdr(struct net_pkt *pkt,
					   union net_proto_header *proto_hdr)
{
	pkt->vpn.proto_hdr = *proto_hdr;
}

static inline int net_pkt_vpn_peer_id(struct net_pkt *pkt)
{
	return pkt->vpn.peer_id;
}

static inline void net_pkt_set_vpn_peer_id(struct net_pkt *pkt,
					   int peer_id)
{
	pkt->vpn.peer_id = peer_id;
}
#endif /* CONFIG_NET_VPN */

static inline uint8_t net_pkt_family(struct net_pkt *pkt)
{
	return pkt->family;
}

static inline void net_pkt_set_family(struct net_pkt *pkt, uint8_t family)
{
	pkt->family = family;
}

static inline bool net_pkt_is_ptp(struct net_pkt *pkt)
{
	return !!(pkt->ptp_pkt);
}

static inline void net_pkt_set_ptp(struct net_pkt *pkt, bool is_ptp)
{
	pkt->ptp_pkt = is_ptp;
}

static inline bool net_pkt_is_tx_timestamping(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_PKT_TIMESTAMP)
	return !!(pkt->tx_timestamping);
#else
	ARG_UNUSED(pkt);

	return false;
#endif
}

static inline void net_pkt_set_tx_timestamping(struct net_pkt *pkt, bool is_timestamping)
{
#if defined(CONFIG_NET_PKT_TIMESTAMP)
	pkt->tx_timestamping = is_timestamping;
#else
	ARG_UNUSED(pkt);
	ARG_UNUSED(is_timestamping);
#endif
}

static inline bool net_pkt_is_rx_timestamping(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_PKT_TIMESTAMP)
	return !!(pkt->rx_timestamping);
#else
	ARG_UNUSED(pkt);

	return false;
#endif
}

static inline void net_pkt_set_rx_timestamping(struct net_pkt *pkt, bool is_timestamping)
{
#if defined(CONFIG_NET_PKT_TIMESTAMP)
	pkt->rx_timestamping = is_timestamping;
#else
	ARG_UNUSED(pkt);
	ARG_UNUSED(is_timestamping);
#endif
}

static inline bool net_pkt_is_captured(struct net_pkt *pkt)
{
	return !!(pkt->captured);
}

static inline void net_pkt_set_captured(struct net_pkt *pkt, bool is_captured)
{
	pkt->captured = is_captured;
}

static inline bool net_pkt_is_l2_bridged(struct net_pkt *pkt)
{
	return IS_ENABLED(CONFIG_NET_ETHERNET_BRIDGE) ? !!(pkt->l2_bridged) : 0;
}

static inline void net_pkt_set_l2_bridged(struct net_pkt *pkt, bool is_l2_bridged)
{
	if (IS_ENABLED(CONFIG_NET_ETHERNET_BRIDGE)) {
		pkt->l2_bridged = is_l2_bridged;
	}
}

static inline bool net_pkt_is_l2_processed(struct net_pkt *pkt)
{
	return !!(pkt->l2_processed);
}

static inline void net_pkt_set_l2_processed(struct net_pkt *pkt,
					    bool is_l2_processed)
{
	pkt->l2_processed = is_l2_processed;
}

static inline bool net_pkt_is_chksum_done(struct net_pkt *pkt)
{
	return !!(pkt->chksum_done);
}

static inline void net_pkt_set_chksum_done(struct net_pkt *pkt,
					   bool is_chksum_done)
{
	pkt->chksum_done = is_chksum_done;
}

static inline uint8_t net_pkt_ip_hdr_len(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_IP)
	return pkt->ip_hdr_len;
#else
	ARG_UNUSED(pkt);

	return 0;
#endif
}

static inline void net_pkt_set_ip_hdr_len(struct net_pkt *pkt, uint8_t len)
{
#if defined(CONFIG_NET_IP)
	pkt->ip_hdr_len = len;
#else
	ARG_UNUSED(pkt);
	ARG_UNUSED(len);
#endif
}

static inline uint8_t net_pkt_ip_dscp(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_IP_DSCP_ECN)
	return pkt->ip_dscp;
#else
	ARG_UNUSED(pkt);

	return 0;
#endif
}

static inline void net_pkt_set_ip_dscp(struct net_pkt *pkt, uint8_t dscp)
{
#if defined(CONFIG_NET_IP_DSCP_ECN)
	pkt->ip_dscp = dscp;
#else
	ARG_UNUSED(pkt);
	ARG_UNUSED(dscp);
#endif
}

static inline uint8_t net_pkt_ip_ecn(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_IP_DSCP_ECN)
	return pkt->ip_ecn;
#else
	ARG_UNUSED(pkt);

	return 0;
#endif
}

static inline void net_pkt_set_ip_ecn(struct net_pkt *pkt, uint8_t ecn)
{
#if defined(CONFIG_NET_IP_DSCP_ECN)
	pkt->ip_ecn = ecn;
#else
	ARG_UNUSED(pkt);
	ARG_UNUSED(ecn);
#endif
}

static inline uint8_t net_pkt_eof(struct net_pkt *pkt)
{
	return pkt->eof;
}

static inline void net_pkt_set_eof(struct net_pkt *pkt, bool eof)
{
	pkt->eof = eof;
}

static inline bool net_pkt_forwarding(struct net_pkt *pkt)
{
	return !!(pkt->forwarding);
}

static inline void net_pkt_set_forwarding(struct net_pkt *pkt, bool forward)
{
	pkt->forwarding = forward;
}

#if defined(CONFIG_NET_IPV4)
static inline uint8_t net_pkt_ipv4_ttl(struct net_pkt *pkt)
{
	return pkt->ipv4_ttl;
}

static inline void net_pkt_set_ipv4_ttl(struct net_pkt *pkt,
					uint8_t ttl)
{
	pkt->ipv4_ttl = ttl;
}

static inline uint8_t net_pkt_ipv4_opts_len(struct net_pkt *pkt)
{
	return pkt->ipv4_opts_len;
}

static inline void net_pkt_set_ipv4_opts_len(struct net_pkt *pkt,
					     uint8_t opts_len)
{
	pkt->ipv4_opts_len = opts_len;
}
#else
static inline uint8_t net_pkt_ipv4_ttl(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv4_ttl(struct net_pkt *pkt,
					uint8_t ttl)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(ttl);
}

static inline uint8_t net_pkt_ipv4_opts_len(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);
	return 0;
}

static inline void net_pkt_set_ipv4_opts_len(struct net_pkt *pkt,
					     uint8_t opts_len)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(opts_len);
}
#endif

#if defined(CONFIG_NET_IPV6)
static inline uint8_t net_pkt_ipv6_ext_opt_len(struct net_pkt *pkt)
{
	return pkt->ipv6_ext_opt_len;
}

static inline void net_pkt_set_ipv6_ext_opt_len(struct net_pkt *pkt,
						uint8_t len)
{
	pkt->ipv6_ext_opt_len = len;
}

static inline uint8_t net_pkt_ipv6_next_hdr(struct net_pkt *pkt)
{
	return pkt->ipv6_next_hdr;
}

static inline void net_pkt_set_ipv6_next_hdr(struct net_pkt *pkt,
					     uint8_t next_hdr)
{
	pkt->ipv6_next_hdr = next_hdr;
}

static inline uint16_t net_pkt_ipv6_ext_len(struct net_pkt *pkt)
{
	return pkt->ipv6_ext_len;
}

static inline void net_pkt_set_ipv6_ext_len(struct net_pkt *pkt, uint16_t len)
{
	pkt->ipv6_ext_len = len;
}

static inline uint16_t net_pkt_ipv6_hdr_prev(struct net_pkt *pkt)
{
	return pkt->ipv6_prev_hdr_start;
}

static inline void net_pkt_set_ipv6_hdr_prev(struct net_pkt *pkt,
					     uint16_t offset)
{
	pkt->ipv6_prev_hdr_start = offset;
}

static inline uint8_t net_pkt_ipv6_hop_limit(struct net_pkt *pkt)
{
	return pkt->ipv6_hop_limit;
}

static inline void net_pkt_set_ipv6_hop_limit(struct net_pkt *pkt,
					      uint8_t hop_limit)
{
	pkt->ipv6_hop_limit = hop_limit;
}
#else /* CONFIG_NET_IPV6 */
static inline uint8_t net_pkt_ipv6_ext_opt_len(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv6_ext_opt_len(struct net_pkt *pkt,
						uint8_t len)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(len);
}

static inline uint8_t net_pkt_ipv6_next_hdr(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv6_next_hdr(struct net_pkt *pkt,
					     uint8_t next_hdr)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(next_hdr);
}

static inline uint16_t net_pkt_ipv6_ext_len(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv6_ext_len(struct net_pkt *pkt, uint16_t len)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(len);
}

static inline uint16_t net_pkt_ipv6_hdr_prev(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv6_hdr_prev(struct net_pkt *pkt,
					     uint16_t offset)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(offset);
}

static inline uint8_t net_pkt_ipv6_hop_limit(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv6_hop_limit(struct net_pkt *pkt,
					      uint8_t hop_limit)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(hop_limit);
}
#endif /* CONFIG_NET_IPV6 */

static inline uint16_t net_pkt_ip_opts_len(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_IPV6)
	return pkt->ipv6_ext_len;
#elif defined(CONFIG_NET_IPV4)
	return pkt->ipv4_opts_len;
#else
	ARG_UNUSED(pkt);

	return 0;
#endif
}

#if defined(CONFIG_NET_IPV4_PMTU)
static inline bool net_pkt_ipv4_pmtu(struct net_pkt *pkt)
{
	return !!pkt->ipv4_pmtu;
}

static inline void net_pkt_set_ipv4_pmtu(struct net_pkt *pkt, bool value)
{
	pkt->ipv4_pmtu = value;
}
#else
static inline bool net_pkt_ipv4_pmtu(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return false;
}

static inline void net_pkt_set_ipv4_pmtu(struct net_pkt *pkt, bool value)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(value);
}
#endif /* CONFIG_NET_IPV4_PMTU */

#if defined(CONFIG_NET_IPV4_FRAGMENT)
static inline uint16_t net_pkt_ipv4_fragment_offset(struct net_pkt *pkt)
{
	return (pkt->ipv4_fragment.flags & NET_IPV4_FRAGH_OFFSET_MASK) * 8;
}

static inline bool net_pkt_ipv4_fragment_more(struct net_pkt *pkt)
{
	return (pkt->ipv4_fragment.flags & NET_IPV4_MORE_FRAG_MASK) != 0;
}

static inline void net_pkt_set_ipv4_fragment_flags(struct net_pkt *pkt, uint16_t flags)
{
	pkt->ipv4_fragment.flags = flags;
}

static inline uint32_t net_pkt_ipv4_fragment_id(struct net_pkt *pkt)
{
	return pkt->ipv4_fragment.id;
}

static inline void net_pkt_set_ipv4_fragment_id(struct net_pkt *pkt, uint32_t id)
{
	pkt->ipv4_fragment.id = id;
}
#else /* CONFIG_NET_IPV4_FRAGMENT */
static inline uint16_t net_pkt_ipv4_fragment_offset(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline bool net_pkt_ipv4_fragment_more(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv4_fragment_flags(struct net_pkt *pkt, uint16_t flags)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(flags);
}

static inline uint32_t net_pkt_ipv4_fragment_id(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv4_fragment_id(struct net_pkt *pkt, uint32_t id)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(id);
}
#endif /* CONFIG_NET_IPV4_FRAGMENT */

#if defined(CONFIG_NET_IPV6_FRAGMENT)
static inline uint16_t net_pkt_ipv6_fragment_start(struct net_pkt *pkt)
{
	return pkt->ipv6_fragment.hdr_start;
}

static inline void net_pkt_set_ipv6_fragment_start(struct net_pkt *pkt,
						   uint16_t start)
{
	pkt->ipv6_fragment.hdr_start = start;
}

static inline uint16_t net_pkt_ipv6_fragment_offset(struct net_pkt *pkt)
{
	return pkt->ipv6_fragment.flags & NET_IPV6_FRAGH_OFFSET_MASK;
}
static inline bool net_pkt_ipv6_fragment_more(struct net_pkt *pkt)
{
	return (pkt->ipv6_fragment.flags & 0x01) != 0;
}

static inline void net_pkt_set_ipv6_fragment_flags(struct net_pkt *pkt,
						   uint16_t flags)
{
	pkt->ipv6_fragment.flags = flags;
}

static inline uint32_t net_pkt_ipv6_fragment_id(struct net_pkt *pkt)
{
	return pkt->ipv6_fragment.id;
}

static inline void net_pkt_set_ipv6_fragment_id(struct net_pkt *pkt,
						uint32_t id)
{
	pkt->ipv6_fragment.id = id;
}
#else /* CONFIG_NET_IPV6_FRAGMENT */
static inline uint16_t net_pkt_ipv6_fragment_start(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv6_fragment_start(struct net_pkt *pkt,
						   uint16_t start)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(start);
}

static inline uint16_t net_pkt_ipv6_fragment_offset(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline bool net_pkt_ipv6_fragment_more(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv6_fragment_flags(struct net_pkt *pkt,
						   uint16_t flags)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(flags);
}

static inline uint32_t net_pkt_ipv6_fragment_id(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_ipv6_fragment_id(struct net_pkt *pkt,
						uint32_t id)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(id);
}
#endif /* CONFIG_NET_IPV6_FRAGMENT */

static inline bool net_pkt_is_loopback(struct net_pkt *pkt)
{
	return !!(pkt->loopback);
}

static inline void net_pkt_set_loopback(struct net_pkt *pkt,
					bool loopback)
{
	pkt->loopback = loopback;
}

#if defined(CONFIG_NET_IP_FRAGMENT)
static inline bool net_pkt_is_ip_reassembled(struct net_pkt *pkt)
{
	return !!(pkt->ip_reassembled);
}

static inline void net_pkt_set_ip_reassembled(struct net_pkt *pkt,
					      bool reassembled)
{
	pkt->ip_reassembled = reassembled;
}
#else /* CONFIG_NET_IP_FRAGMENT */
static inline bool net_pkt_is_ip_reassembled(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return false;
}

static inline void net_pkt_set_ip_reassembled(struct net_pkt *pkt,
					      bool reassembled)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(reassembled);
}
#endif /* CONFIG_NET_IP_FRAGMENT */

static inline uint8_t net_pkt_priority(struct net_pkt *pkt)
{
	return pkt->priority;
}

static inline void net_pkt_set_priority(struct net_pkt *pkt,
					uint8_t priority)
{
	pkt->priority = priority;
}

#if defined(CONFIG_NET_CAPTURE_COOKED_MODE)
static inline bool net_pkt_is_cooked_mode(struct net_pkt *pkt)
{
	return pkt->cooked_mode_pkt;
}

static inline void net_pkt_set_cooked_mode(struct net_pkt *pkt, bool value)
{
	pkt->cooked_mode_pkt = value;
}
#else
static inline bool net_pkt_is_cooked_mode(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return false;
}

static inline void net_pkt_set_cooked_mode(struct net_pkt *pkt, bool value)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(value);
}
#endif /* CONFIG_NET_CAPTURE_COOKED_MODE */

#if defined(CONFIG_NET_VLAN)
static inline uint16_t net_pkt_vlan_tag(struct net_pkt *pkt)
{
	return net_eth_vlan_get_vid(pkt->vlan_tci);
}

static inline void net_pkt_set_vlan_tag(struct net_pkt *pkt, uint16_t tag)
{
	pkt->vlan_tci = net_eth_vlan_set_vid(pkt->vlan_tci, tag);
}

static inline uint8_t net_pkt_vlan_priority(struct net_pkt *pkt)
{
	return net_eth_vlan_get_pcp(pkt->vlan_tci);
}

static inline void net_pkt_set_vlan_priority(struct net_pkt *pkt,
					     uint8_t priority)
{
	pkt->vlan_tci = net_eth_vlan_set_pcp(pkt->vlan_tci, priority);
}

static inline bool net_pkt_vlan_dei(struct net_pkt *pkt)
{
	return net_eth_vlan_get_dei(pkt->vlan_tci);
}

static inline void net_pkt_set_vlan_dei(struct net_pkt *pkt, bool dei)
{
	pkt->vlan_tci = net_eth_vlan_set_dei(pkt->vlan_tci, dei);
}

static inline void net_pkt_set_vlan_tci(struct net_pkt *pkt, uint16_t tci)
{
	pkt->vlan_tci = tci;
}

static inline uint16_t net_pkt_vlan_tci(struct net_pkt *pkt)
{
	return pkt->vlan_tci;
}
#else
static inline uint16_t net_pkt_vlan_tag(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return NET_VLAN_TAG_UNSPEC;
}

static inline void net_pkt_set_vlan_tag(struct net_pkt *pkt, uint16_t tag)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(tag);
}

static inline uint8_t net_pkt_vlan_priority(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline bool net_pkt_vlan_dei(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return false;
}

static inline void net_pkt_set_vlan_dei(struct net_pkt *pkt, bool dei)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(dei);
}

static inline uint16_t net_pkt_vlan_tci(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return NET_VLAN_TAG_UNSPEC; /* assumes priority is 0 */
}

static inline void net_pkt_set_vlan_tci(struct net_pkt *pkt, uint16_t tci)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(tci);
}
#endif

#if defined(CONFIG_NET_PKT_TIMESTAMP) || defined(CONFIG_NET_PKT_TXTIME)
static inline struct net_ptp_time *net_pkt_timestamp(struct net_pkt *pkt)
{
	return &pkt->timestamp;
}

static inline void net_pkt_set_timestamp(struct net_pkt *pkt,
					 struct net_ptp_time *timestamp)
{
	pkt->timestamp.second = timestamp->second;
	pkt->timestamp.nanosecond = timestamp->nanosecond;
}

static inline net_time_t net_pkt_timestamp_ns(struct net_pkt *pkt)
{
	return net_ptp_time_to_ns(&pkt->timestamp);
}

static inline void net_pkt_set_timestamp_ns(struct net_pkt *pkt, net_time_t timestamp)
{
	pkt->timestamp = ns_to_net_ptp_time(timestamp);
}
#else
static inline struct net_ptp_time *net_pkt_timestamp(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return NULL;
}

static inline void net_pkt_set_timestamp(struct net_pkt *pkt,
					 struct net_ptp_time *timestamp)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(timestamp);
}

static inline net_time_t net_pkt_timestamp_ns(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_set_timestamp_ns(struct net_pkt *pkt, net_time_t timestamp)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(timestamp);
}
#endif /* CONFIG_NET_PKT_TIMESTAMP || CONFIG_NET_PKT_TXTIME */

#if defined(CONFIG_NET_PKT_RXTIME_STATS) || defined(CONFIG_NET_PKT_TXTIME_STATS) || \
	defined(CONFIG_TRACING_NET_CORE)

static inline uint32_t net_pkt_create_time(struct net_pkt *pkt)
{
	return pkt->create_time;
}

static inline void net_pkt_set_create_time(struct net_pkt *pkt,
					   uint32_t create_time)
{
	pkt->create_time = create_time;
}
#else
static inline uint32_t net_pkt_create_time(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0U;
}

static inline void net_pkt_set_create_time(struct net_pkt *pkt,
					   uint32_t create_time)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(create_time);
}
#endif /* CONFIG_NET_PKT_RXTIME_STATS || CONFIG_NET_PKT_TXTIME_STATS ||
	* CONFIG_TRACING_NET_CORE
	*/

#if defined(CONFIG_NET_PKT_TXTIME_STATS_DETAIL) || \
	defined(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)
static inline uint32_t *net_pkt_stats_tick(struct net_pkt *pkt)
{
	return pkt->detail.stat;
}

static inline int net_pkt_stats_tick_count(struct net_pkt *pkt)
{
	return pkt->detail.count;
}

static inline void net_pkt_stats_tick_reset(struct net_pkt *pkt)
{
	memset(&pkt->detail, 0, sizeof(pkt->detail));
}

static ALWAYS_INLINE void net_pkt_set_stats_tick(struct net_pkt *pkt,
						 uint32_t tick)
{
	if (pkt->detail.count >= NET_PKT_DETAIL_STATS_COUNT) {
		NET_ERR("Detail stats count overflow (%d >= %d)",
			pkt->detail.count, NET_PKT_DETAIL_STATS_COUNT);
		return;
	}

	pkt->detail.stat[pkt->detail.count++] = tick;
}

#define net_pkt_set_tx_stats_tick(pkt, tick) net_pkt_set_stats_tick(pkt, tick)
#define net_pkt_set_rx_stats_tick(pkt, tick) net_pkt_set_stats_tick(pkt, tick)
#else
static inline uint32_t *net_pkt_stats_tick(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return NULL;
}

static inline int net_pkt_stats_tick_count(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline void net_pkt_stats_tick_reset(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);
}

static inline void net_pkt_set_stats_tick(struct net_pkt *pkt, uint32_t tick)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(tick);
}

#define net_pkt_set_tx_stats_tick(pkt, tick)
#define net_pkt_set_rx_stats_tick(pkt, tick)
#endif /* CONFIG_NET_PKT_TXTIME_STATS_DETAIL ||
	  CONFIG_NET_PKT_RXTIME_STATS_DETAIL */

static inline uint8_t *net_pkt_data(struct net_pkt *pkt)
{
	return pkt->frags->data;
}

static inline uint8_t *net_pkt_ip_data(struct net_pkt *pkt)
{
	return pkt->frags->data;
}

static inline bool net_pkt_is_empty(struct net_pkt *pkt)
{
	return !pkt->buffer || !net_pkt_data(pkt) || pkt->buffer->len == 0;
}

static inline struct net_linkaddr *net_pkt_lladdr_src(struct net_pkt *pkt)
{
	return &pkt->lladdr_src;
}

static inline struct net_linkaddr *net_pkt_lladdr_dst(struct net_pkt *pkt)
{
	return &pkt->lladdr_dst;
}

static inline void net_pkt_lladdr_swap(struct net_pkt *pkt)
{
	struct net_linkaddr tmp;

	memcpy(tmp.addr,
	       net_pkt_lladdr_src(pkt)->addr,
	       net_pkt_lladdr_src(pkt)->len);
	memcpy(net_pkt_lladdr_src(pkt)->addr,
	       net_pkt_lladdr_dst(pkt)->addr,
	       net_pkt_lladdr_dst(pkt)->len);
	memcpy(net_pkt_lladdr_dst(pkt)->addr,
	       tmp.addr,
	       net_pkt_lladdr_src(pkt)->len);
}

static inline void net_pkt_lladdr_clear(struct net_pkt *pkt)
{
	(void)net_linkaddr_clear(net_pkt_lladdr_src(pkt));
	(void)net_linkaddr_clear(net_pkt_lladdr_dst(pkt));
}

static inline uint16_t net_pkt_ll_proto_type(struct net_pkt *pkt)
{
	return pkt->ll_proto_type;
}

static inline void net_pkt_set_ll_proto_type(struct net_pkt *pkt, uint16_t type)
{
	pkt->ll_proto_type = type;
}

#if defined(CONFIG_NET_IPV4_ACD)
static inline bool net_pkt_ipv4_acd(struct net_pkt *pkt)
{
	return !!(pkt->ipv4_acd_arp_msg);
}

static inline void net_pkt_set_ipv4_acd(struct net_pkt *pkt,
					bool is_acd_arp_msg)
{
	pkt->ipv4_acd_arp_msg = is_acd_arp_msg;
}
#else /* CONFIG_NET_IPV4_ACD */
static inline bool net_pkt_ipv4_acd(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return false;
}

static inline void net_pkt_set_ipv4_acd(struct net_pkt *pkt,
					bool is_acd_arp_msg)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(is_acd_arp_msg);
}
#endif /* CONFIG_NET_IPV4_ACD */

#if defined(CONFIG_NET_LLDP)
static inline bool net_pkt_is_lldp(struct net_pkt *pkt)
{
	return !!(pkt->lldp_pkt);
}

static inline void net_pkt_set_lldp(struct net_pkt *pkt, bool is_lldp)
{
	pkt->lldp_pkt = is_lldp;
}
#else
static inline bool net_pkt_is_lldp(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return false;
}

static inline void net_pkt_set_lldp(struct net_pkt *pkt, bool is_lldp)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(is_lldp);
}
#endif /* CONFIG_NET_LLDP */

#if defined(CONFIG_NET_L2_PPP)
static inline bool net_pkt_is_ppp(struct net_pkt *pkt)
{
	return !!(pkt->ppp_msg);
}

static inline void net_pkt_set_ppp(struct net_pkt *pkt,
				   bool is_ppp_msg)
{
	pkt->ppp_msg = is_ppp_msg;
}
#else /* CONFIG_NET_L2_PPP */
static inline bool net_pkt_is_ppp(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return false;
}

static inline void net_pkt_set_ppp(struct net_pkt *pkt,
				   bool is_ppp_msg)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(is_ppp_msg);
}
#endif /* CONFIG_NET_L2_PPP */

#if defined(CONFIG_NET_PKT_CONTROL_BLOCK)
static inline void *net_pkt_cb(struct net_pkt *pkt)
{
	return &pkt->cb;
}
#else
static inline void *net_pkt_cb(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return NULL;
}
#endif

#define NET_IPV6_HDR(pkt) ((struct net_ipv6_hdr *)net_pkt_ip_data(pkt))
#define NET_IPV4_HDR(pkt) ((struct net_ipv4_hdr *)net_pkt_ip_data(pkt))

static inline void net_pkt_set_src_ipv6_addr(struct net_pkt *pkt)
{
	net_if_ipv6_select_src_addr(net_context_get_iface(
					    net_pkt_context(pkt)),
				    (struct in6_addr *)NET_IPV6_HDR(pkt)->src);
}

static inline void net_pkt_set_overwrite(struct net_pkt *pkt, bool overwrite)
{
	pkt->overwrite = overwrite;
}

static inline bool net_pkt_is_being_overwritten(struct net_pkt *pkt)
{
	return !!(pkt->overwrite);
}

#ifdef CONFIG_NET_PKT_FILTER

bool net_pkt_filter_send_ok(struct net_pkt *pkt);
bool net_pkt_filter_recv_ok(struct net_pkt *pkt);

#else

static inline bool net_pkt_filter_send_ok(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return true;
}

static inline bool net_pkt_filter_recv_ok(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return true;
}

#endif /* CONFIG_NET_PKT_FILTER */

#if defined(CONFIG_NET_PKT_FILTER) && \
	(defined(CONFIG_NET_PKT_FILTER_IPV4_HOOK) || defined(CONFIG_NET_PKT_FILTER_IPV6_HOOK))

bool net_pkt_filter_ip_recv_ok(struct net_pkt *pkt);

#else

static inline bool net_pkt_filter_ip_recv_ok(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return true;
}

#endif /* CONFIG_NET_PKT_FILTER_IPV4_HOOK || CONFIG_NET_PKT_FILTER_IPV6_HOOK */

#if defined(CONFIG_NET_PKT_FILTER) && defined(CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK)

bool net_pkt_filter_local_in_recv_ok(struct net_pkt *pkt);

#else

static inline bool net_pkt_filter_local_in_recv_ok(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return true;
}

#endif /* CONFIG_NET_PKT_FILTER && CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK */

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_L2_IPIP)
static inline struct sockaddr *net_pkt_remote_address(struct net_pkt *pkt)
{
	return &pkt->remote;
}

static inline void net_pkt_set_remote_address(struct net_pkt *pkt,
					      struct sockaddr *address,
					      socklen_t len)
{
	memcpy(&pkt->remote, address, len);
}
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_L2_IPIP */

/* @endcond */

/**
 * @brief Create a net_pkt slab
 *
 * A net_pkt slab is used to store meta-information about
 * network packets. It must be coupled with a data fragment pool
 * (@ref NET_PKT_DATA_POOL_DEFINE) used to store the actual
 * packet data. The macro can be used by an application to define
 * additional custom per-context TX packet slabs (see
 * net_context_setup_pools()).
 *
 * @param name Name of the slab.
 * @param count Number of net_pkt in this slab.
 */
#define NET_PKT_SLAB_DEFINE(name, count)				\
	K_MEM_SLAB_DEFINE(name, sizeof(struct net_pkt), count, 4);      \
	NET_PKT_ALLOC_STATS_DEFINE(pkt_alloc_stats_##name, name)

/** @cond INTERNAL_HIDDEN */

/* Backward compatibility macro */
#define NET_PKT_TX_SLAB_DEFINE(name, count) NET_PKT_SLAB_DEFINE(name, count)

/** @endcond */

/**
 * @brief Create a data fragment net_buf pool
 *
 * A net_buf pool is used to store actual data for
 * network packets. It must be coupled with a net_pkt slab
 * (@ref NET_PKT_SLAB_DEFINE) used to store the packet
 * meta-information. The macro can be used by an application to
 * define additional custom per-context TX packet pools (see
 * net_context_setup_pools()).
 *
 * @param name Name of the pool.
 * @param count Number of net_buf in this pool.
 */
#define NET_PKT_DATA_POOL_DEFINE(name, count)				\
	NET_BUF_POOL_DEFINE(name, count, CONFIG_NET_BUF_DATA_SIZE,	\
			    0, NULL)

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC) || \
	(CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG)
#define NET_PKT_DEBUG_ENABLED
#endif

#if defined(NET_PKT_DEBUG_ENABLED)

/* Debug versions of the net_pkt functions that are used when tracking
 * buffer usage.
 */

struct net_buf *net_pkt_get_reserve_data_debug(struct net_buf_pool *pool,
					       size_t min_len,
					       k_timeout_t timeout,
					       const char *caller,
					       int line);

#define net_pkt_get_reserve_data(pool, min_len, timeout)				\
	net_pkt_get_reserve_data_debug(pool, min_len, timeout, __func__, __LINE__)

struct net_buf *net_pkt_get_reserve_rx_data_debug(size_t min_len,
						  k_timeout_t timeout,
						  const char *caller,
						  int line);
#define net_pkt_get_reserve_rx_data(min_len, timeout)				\
	net_pkt_get_reserve_rx_data_debug(min_len, timeout, __func__, __LINE__)

struct net_buf *net_pkt_get_reserve_tx_data_debug(size_t min_len,
						  k_timeout_t timeout,
						  const char *caller,
						  int line);
#define net_pkt_get_reserve_tx_data(min_len, timeout)				\
	net_pkt_get_reserve_tx_data_debug(min_len, timeout, __func__, __LINE__)

struct net_buf *net_pkt_get_frag_debug(struct net_pkt *pkt, size_t min_len,
				       k_timeout_t timeout,
				       const char *caller, int line);
#define net_pkt_get_frag(pkt, min_len, timeout)					\
	net_pkt_get_frag_debug(pkt, min_len, timeout, __func__, __LINE__)

void net_pkt_unref_debug(struct net_pkt *pkt, const char *caller, int line);
#define net_pkt_unref(pkt) net_pkt_unref_debug(pkt, __func__, __LINE__)

struct net_pkt *net_pkt_ref_debug(struct net_pkt *pkt, const char *caller,
				  int line);
#define net_pkt_ref(pkt) net_pkt_ref_debug(pkt, __func__, __LINE__)

struct net_buf *net_pkt_frag_ref_debug(struct net_buf *frag,
				       const char *caller, int line);
#define net_pkt_frag_ref(frag) net_pkt_frag_ref_debug(frag, __func__, __LINE__)

void net_pkt_frag_unref_debug(struct net_buf *frag,
			      const char *caller, int line);
#define net_pkt_frag_unref(frag)				\
	net_pkt_frag_unref_debug(frag, __func__, __LINE__)

struct net_buf *net_pkt_frag_del_debug(struct net_pkt *pkt,
				       struct net_buf *parent,
				       struct net_buf *frag,
				       const char *caller, int line);
#define net_pkt_frag_del(pkt, parent, frag)				\
	net_pkt_frag_del_debug(pkt, parent, frag, __func__, __LINE__)

void net_pkt_frag_add_debug(struct net_pkt *pkt, struct net_buf *frag,
			    const char *caller, int line);
#define net_pkt_frag_add(pkt, frag)				\
	net_pkt_frag_add_debug(pkt, frag, __func__, __LINE__)

void net_pkt_frag_insert_debug(struct net_pkt *pkt, struct net_buf *frag,
			       const char *caller, int line);
#define net_pkt_frag_insert(pkt, frag)					\
	net_pkt_frag_insert_debug(pkt, frag, __func__, __LINE__)
#endif /* CONFIG_NET_DEBUG_NET_PKT_ALLOC ||
	* CONFIG_NET_PKT_LOG_LEVEL >= LOG_LEVEL_DBG
	*/
/** @endcond */

#if defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Print fragment list and the fragment sizes
 *
 * @details Only available if debugging is activated.
 *
 * @param pkt Network pkt.
 */
void net_pkt_print_frags(struct net_pkt *pkt);
#else
#define net_pkt_print_frags(pkt)
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Get a data buffer from a given pool.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param pool The net_buf pool to use.
 * @param min_len Minimum length of the requested fragment.
 * @param timeout Affects the action taken should the net buf pool be empty.
 *        If K_NO_WAIT, then return immediately. If K_FOREVER, then
 *        wait as long as necessary. Otherwise, wait up to the specified time.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_pkt_get_reserve_data(struct net_buf_pool *pool,
					 size_t min_len, k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Get RX DATA buffer from pool.
 * Normally you should use net_pkt_get_frag() instead.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param min_len Minimum length of the requested fragment.
 * @param timeout Affects the action taken should the net buf pool be empty.
 *        If K_NO_WAIT, then return immediately. If K_FOREVER, then
 *        wait as long as necessary. Otherwise, wait up to the specified time.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_pkt_get_reserve_rx_data(size_t min_len, k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Get TX DATA buffer from pool.
 * Normally you should use net_pkt_get_frag() instead.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param min_len Minimum length of the requested fragment.
 * @param timeout Affects the action taken should the net buf pool be empty.
 *        If K_NO_WAIT, then return immediately. If K_FOREVER, then
 *        wait as long as necessary. Otherwise, wait up to the specified time.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_pkt_get_reserve_tx_data(size_t min_len, k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Get a data fragment that might be from user specific
 * buffer pool or from global DATA pool.
 *
 * @param pkt Network packet.
 * @param min_len Minimum length of the requested fragment.
 * @param timeout Affects the action taken should the net buf pool be empty.
 *        If K_NO_WAIT, then return immediately. If K_FOREVER, then
 *        wait as long as necessary. Otherwise, wait up to the specified time.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_pkt_get_frag(struct net_pkt *pkt, size_t min_len,
				 k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Place packet back into the available packets slab
 *
 * @details Releases the packet to other use. This needs to be
 * called by application after it has finished with the packet.
 *
 * @param pkt Network packet to release.
 *
 */
void net_pkt_unref(struct net_pkt *pkt);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Increase the packet ref count
 *
 * @details Mark the packet to be used still.
 *
 * @param pkt Network packet to ref.
 *
 * @return Network packet if successful, NULL otherwise.
 */
struct net_pkt *net_pkt_ref(struct net_pkt *pkt);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Increase the packet fragment ref count
 *
 * @details Mark the fragment to be used still.
 *
 * @param frag Network fragment to ref.
 *
 * @return a pointer on the referenced Network fragment.
 */
struct net_buf *net_pkt_frag_ref(struct net_buf *frag);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Decrease the packet fragment ref count
 *
 * @param frag Network fragment to unref.
 */
void net_pkt_frag_unref(struct net_buf *frag);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Delete existing fragment from a packet
 *
 * @param pkt Network packet from which frag belongs to.
 * @param parent parent fragment of frag, or NULL if none.
 * @param frag Fragment to delete.
 *
 * @return Pointer to the following fragment, or NULL if it had no
 *         further fragments.
 */
struct net_buf *net_pkt_frag_del(struct net_pkt *pkt,
				 struct net_buf *parent,
				 struct net_buf *frag);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Add a fragment to a packet at the end of its fragment list
 *
 * @param pkt pkt Network packet where to add the fragment
 * @param frag Fragment to add
 */
void net_pkt_frag_add(struct net_pkt *pkt, struct net_buf *frag);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Insert a fragment to a packet at the beginning of its fragment list
 *
 * @param pkt pkt Network packet where to insert the fragment
 * @param frag Fragment to insert
 */
void net_pkt_frag_insert(struct net_pkt *pkt, struct net_buf *frag);
#endif

/**
 * @brief Compact the fragment list of a packet.
 *
 * @details After this there is no more any free space in individual fragments.
 * @param pkt Network packet.
 */
void net_pkt_compact(struct net_pkt *pkt);

/**
 * @brief Get information about predefined RX, TX and DATA pools.
 *
 * @param rx Pointer to RX pool is returned.
 * @param tx Pointer to TX pool is returned.
 * @param rx_data Pointer to RX DATA pool is returned.
 * @param tx_data Pointer to TX DATA pool is returned.
 */
void net_pkt_get_info(struct k_mem_slab **rx,
		      struct k_mem_slab **tx,
		      struct net_buf_pool **rx_data,
		      struct net_buf_pool **tx_data);

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
/**
 * @brief Debug helper to print out the buffer allocations
 */
void net_pkt_print(void);

typedef void (*net_pkt_allocs_cb_t)(struct net_pkt *pkt,
				    struct net_buf *buf,
				    const char *func_alloc,
				    int line_alloc,
				    const char *func_free,
				    int line_free,
				    bool in_use,
				    void *user_data);

void net_pkt_allocs_foreach(net_pkt_allocs_cb_t cb, void *user_data);

const char *net_pkt_slab2str(struct k_mem_slab *slab);
const char *net_pkt_pool2str(struct net_buf_pool *pool);

#else
#define net_pkt_print(...)
#endif /* CONFIG_NET_DEBUG_NET_PKT_ALLOC */

/* New allocator, and API are defined below.
 * This will be simpler when time will come to get rid of former API above.
 */
#if defined(NET_PKT_DEBUG_ENABLED)

struct net_pkt *net_pkt_alloc_debug(k_timeout_t timeout,
				    const char *caller, int line);
#define net_pkt_alloc(_timeout)					\
	net_pkt_alloc_debug(_timeout, __func__, __LINE__)

struct net_pkt *net_pkt_alloc_from_slab_debug(struct k_mem_slab *slab,
					      k_timeout_t timeout,
					      const char *caller, int line);
#define net_pkt_alloc_from_slab(_slab, _timeout)			\
	net_pkt_alloc_from_slab_debug(_slab, _timeout, __func__, __LINE__)

struct net_pkt *net_pkt_rx_alloc_debug(k_timeout_t timeout,
				       const char *caller, int line);
#define net_pkt_rx_alloc(_timeout)				\
	net_pkt_rx_alloc_debug(_timeout, __func__, __LINE__)

struct net_pkt *net_pkt_alloc_on_iface_debug(struct net_if *iface,
					     k_timeout_t timeout,
					     const char *caller,
					     int line);
#define net_pkt_alloc_on_iface(_iface, _timeout)			\
	net_pkt_alloc_on_iface_debug(_iface, _timeout, __func__, __LINE__)

struct net_pkt *net_pkt_rx_alloc_on_iface_debug(struct net_if *iface,
						k_timeout_t timeout,
						const char *caller,
						int line);
#define net_pkt_rx_alloc_on_iface(_iface, _timeout)			\
	net_pkt_rx_alloc_on_iface_debug(_iface, _timeout,		\
					__func__, __LINE__)

int net_pkt_alloc_buffer_debug(struct net_pkt *pkt,
			       size_t size,
			       enum net_ip_protocol proto,
			       k_timeout_t timeout,
			       const char *caller, int line);
#define net_pkt_alloc_buffer(_pkt, _size, _proto, _timeout)		\
	net_pkt_alloc_buffer_debug(_pkt, _size, _proto, _timeout,	\
				   __func__, __LINE__)

int net_pkt_alloc_buffer_raw_debug(struct net_pkt *pkt, size_t size,
				   k_timeout_t timeout,
				   const char *caller, int line);
#define net_pkt_alloc_buffer_raw(_pkt, _size, _timeout)	\
	net_pkt_alloc_buffer_raw_debug(_pkt, _size, _timeout,	\
				       __func__, __LINE__)

struct net_pkt *net_pkt_alloc_with_buffer_debug(struct net_if *iface,
						size_t size,
						sa_family_t family,
						enum net_ip_protocol proto,
						k_timeout_t timeout,
						const char *caller,
						int line);
#define net_pkt_alloc_with_buffer(_iface, _size, _family,		\
				  _proto, _timeout)			\
	net_pkt_alloc_with_buffer_debug(_iface, _size, _family,		\
					_proto, _timeout,		\
					__func__, __LINE__)

struct net_pkt *net_pkt_rx_alloc_with_buffer_debug(struct net_if *iface,
						   size_t size,
						   sa_family_t family,
						   enum net_ip_protocol proto,
						   k_timeout_t timeout,
						   const char *caller,
						   int line);
#define net_pkt_rx_alloc_with_buffer(_iface, _size, _family,		\
				     _proto, _timeout)			\
	net_pkt_rx_alloc_with_buffer_debug(_iface, _size, _family,	\
					   _proto, _timeout,		\
					   __func__, __LINE__)

int net_pkt_alloc_buffer_with_reserve_debug(struct net_pkt *pkt,
					    size_t size,
					    size_t reserve,
					    enum net_ip_protocol proto,
					    k_timeout_t timeout,
					    const char *caller,
					    int line);
#define net_pkt_alloc_buffer_with_reserve(_pkt, _size, _reserve, _proto, _timeout) \
	net_pkt_alloc_buffer_with_reserve_debug(_pkt, _size, _reserve, _proto, \
						_timeout, __func__, __LINE__)

#endif /* NET_PKT_DEBUG_ENABLED */
/** @endcond */

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Allocate an initialized net_pkt
 *
 * @details for the time being, 2 pools are used. One for TX and one for RX.
 *          This allocator has to be used for TX.
 *
 * @param timeout Maximum time to wait for an allocation.
 *
 * @return a pointer to a newly allocated net_pkt on success, NULL otherwise.
 */
struct net_pkt *net_pkt_alloc(k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Allocate an initialized net_pkt from a specific slab
 *
 * @details unlike net_pkt_alloc() which uses core slabs, this one will use
 *          an external slab (see NET_PKT_SLAB_DEFINE()).
 *          Do _not_ use it unless you know what you are doing. Basically, only
 *          net_context should be using this, in order to allocate packet and
 *          then buffer on its local slab/pool (if any).
 *
 * @param slab    The slab to use for allocating the packet
 * @param timeout Maximum time to wait for an allocation.
 *
 * @return a pointer to a newly allocated net_pkt on success, NULL otherwise.
 */
struct net_pkt *net_pkt_alloc_from_slab(struct k_mem_slab *slab,
					k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Allocate an initialized net_pkt for RX
 *
 * @details for the time being, 2 pools are used. One for TX and one for RX.
 *          This allocator has to be used for RX.
 *
 * @param timeout Maximum time to wait for an allocation.
 *
 * @return a pointer to a newly allocated net_pkt on success, NULL otherwise.
 */
struct net_pkt *net_pkt_rx_alloc(k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Allocate a network packet for a specific network interface.
 *
 * @param iface The network interface the packet is supposed to go through.
 * @param timeout Maximum time to wait for an allocation.
 *
 * @return a pointer to a newly allocated net_pkt on success, NULL otherwise.
 */
struct net_pkt *net_pkt_alloc_on_iface(struct net_if *iface,
				       k_timeout_t timeout);

/** @cond INTERNAL_HIDDEN */

/* Same as above but specifically for RX packet */
struct net_pkt *net_pkt_rx_alloc_on_iface(struct net_if *iface,
					  k_timeout_t timeout);
/** @endcond */

#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Allocate buffer for a net_pkt
 *
 * @details: such allocator will take into account space necessary for headers,
 *           MTU, and existing buffer (if any). Beware that, due to all these
 *           criteria, the allocated size might be smaller/bigger than
 *           requested one.
 *
 * @param pkt     The network packet requiring buffer to be allocated.
 * @param size    The size of buffer being requested.
 * @param proto   The IP protocol type (can be 0 for none).
 * @param timeout Maximum time to wait for an allocation.
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_alloc_buffer(struct net_pkt *pkt,
			 size_t size,
			 enum net_ip_protocol proto,
			 k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Allocate buffer for a net_pkt and reserve some space in the first net_buf.
 *
 * @details: such allocator will take into account space necessary for headers,
 *           MTU, and existing buffer (if any). Beware that, due to all these
 *           criteria, the allocated size might be smaller/bigger than
 *           requested one.
 *
 * @param pkt     The network packet requiring buffer to be allocated.
 * @param size    The size of buffer being requested.
 * @param reserve The L2 header size to reserve. This can be 0, in which case
 *                the L2 header is placed into a separate net_buf.
 * @param proto   The IP protocol type (can be 0 for none).
 * @param timeout Maximum time to wait for an allocation.
 *
 * @return 0 on success, negative errno code otherwise.
 */
#if !defined(NET_PKT_DEBUG_ENABLED)
int net_pkt_alloc_buffer_with_reserve(struct net_pkt *pkt,
				      size_t size,
				      size_t reserve,
				      enum net_ip_protocol proto,
				      k_timeout_t timeout);
#endif

/**
 * @brief Allocate buffer for a net_pkt, of specified size, w/o any additional
 *        preconditions
 *
 * @details: The actual buffer size may be larger than requested one if fixed
 *           size buffers are in use.
 *
 * @param pkt     The network packet requiring buffer to be allocated.
 * @param size    The size of buffer being requested.
 * @param timeout Maximum time to wait for an allocation.
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_alloc_buffer_raw(struct net_pkt *pkt, size_t size,
			     k_timeout_t timeout);
#endif

#if !defined(NET_PKT_DEBUG_ENABLED)
/**
 * @brief Allocate a network packet and buffer at once
 *
 * @param iface   The network interface the packet is supposed to go through.
 * @param size    The size of buffer.
 * @param family  The family to which the packet belongs.
 * @param proto   The IP protocol type (can be 0 for none).
 * @param timeout Maximum time to wait for an allocation.
 *
 * @return a pointer to a newly allocated net_pkt on success, NULL otherwise.
 */
struct net_pkt *net_pkt_alloc_with_buffer(struct net_if *iface,
					  size_t size,
					  sa_family_t family,
					  enum net_ip_protocol proto,
					  k_timeout_t timeout);

/** @cond INTERNAL_HIDDEN */

/* Same as above but specifically for RX packet */
struct net_pkt *net_pkt_rx_alloc_with_buffer(struct net_if *iface,
					     size_t size,
					     sa_family_t family,
					     enum net_ip_protocol proto,
					     k_timeout_t timeout);

/** @endcond */

#endif

/**
 * @brief Append a buffer in packet
 *
 * @param pkt    Network packet where to append the buffer
 * @param buffer Buffer to append
 */
void net_pkt_append_buffer(struct net_pkt *pkt, struct net_buf *buffer);

/**
 * @brief Get available buffer space from a pkt
 *
 * @note Reserved bytes (headroom) in any of the fragments are not considered to
 *       be available.
 *
 * @param pkt The net_pkt which buffer availability should be evaluated
 *
 * @return the amount of buffer available
 */
size_t net_pkt_available_buffer(struct net_pkt *pkt);

/**
 * @brief Get available buffer space for payload from a pkt
 *
 * @note Reserved bytes (headroom) in any of the fragments are not considered to
 *       be available.
 *
 * @details Unlike net_pkt_available_buffer(), this will take into account
 *          the headers space.
 *
 * @param pkt   The net_pkt which payload buffer availability should
 *              be evaluated
 * @param proto The IP protocol type (can be 0 for none).
 *
 * @return the amount of buffer available for payload
 */
size_t net_pkt_available_payload_buffer(struct net_pkt *pkt,
					enum net_ip_protocol proto);

/**
 * @brief Trim net_pkt buffer
 *
 * @details This will basically check for unused buffers and deallocate
 *          them relevantly
 *
 * @param pkt The net_pkt which buffer will be trimmed
 */
void net_pkt_trim_buffer(struct net_pkt *pkt);

/**
 * @brief Remove @a length bytes from tail of packet
 *
 * @details This function does not take packet cursor into account. It is a
 *          helper to remove unneeded bytes from tail of packet (like appended
 *          CRC). It takes care of buffer deallocation if removed bytes span
 *          whole buffer(s).
 *
 * @param pkt    Network packet
 * @param length Number of bytes to be removed
 *
 * @retval 0       On success.
 * @retval -EINVAL If packet length is shorter than @a length.
 */
int net_pkt_remove_tail(struct net_pkt *pkt, size_t length);

/**
 * @brief Initialize net_pkt cursor
 *
 * @details This will initialize the net_pkt cursor from its buffer.
 *
 * @param pkt The net_pkt whose cursor is going to be initialized
 */
void net_pkt_cursor_init(struct net_pkt *pkt);

/**
 * @brief Backup net_pkt cursor
 *
 * @param pkt    The net_pkt whose cursor is going to be backed up
 * @param backup The cursor where to backup net_pkt cursor
 */
static inline void net_pkt_cursor_backup(struct net_pkt *pkt,
					 struct net_pkt_cursor *backup)
{
	backup->buf = pkt->cursor.buf;
	backup->pos = pkt->cursor.pos;
}

/**
 * @brief Restore net_pkt cursor from a backup
 *
 * @param pkt    The net_pkt whose cursor is going to be restored
 * @param backup The cursor from where to restore net_pkt cursor
 */
static inline void net_pkt_cursor_restore(struct net_pkt *pkt,
					  struct net_pkt_cursor *backup)
{
	pkt->cursor.buf = backup->buf;
	pkt->cursor.pos = backup->pos;
}

/**
 * @brief Returns current position of the cursor
 *
 * @param pkt The net_pkt whose cursor position is going to be returned
 *
 * @return cursor's position
 */
static inline void *net_pkt_cursor_get_pos(struct net_pkt *pkt)
{
	return pkt->cursor.pos;
}

/**
 * @brief Skip some data from a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized
 *          Cursor position will be updated after the operation.
 *          Depending on the value of pkt->overwrite bit, this function
 *          will affect the buffer length or not. If it's true, it will
 *          advance the cursor to the requested length. If it's false,
 *          it will do the same but if the cursor was already also at the
 *          end of existing data, it will increment the buffer length.
 *          So in this case, its behavior is just like net_pkt_write or
 *          net_pkt_memset, difference being that it will not affect the
 *          buffer content itself (which may be just garbage then).
 *
 * @param pkt    The net_pkt whose cursor will be updated to skip given
 *               amount of data from the buffer.
 * @param length Amount of data to skip in the buffer
 *
 * @return 0 in success, negative errno code otherwise.
 */
int net_pkt_skip(struct net_pkt *pkt, size_t length);

/**
 * @brief Memset some data in a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt    The net_pkt whose buffer to fill starting at the current
 *               cursor position.
 * @param byte   The byte to write in memory
 * @param length Amount of data to memset with given byte
 *
 * @return 0 in success, negative errno code otherwise.
 */
int net_pkt_memset(struct net_pkt *pkt, int byte, size_t length);

/**
 * @brief Copy data from a packet into another one.
 *
 * @details Both net_pkt cursors should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          The cursors will be updated after the operation.
 *
 * @param pkt_dst Destination network packet.
 * @param pkt_src Source network packet.
 * @param length  Length of data to be copied.
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_copy(struct net_pkt *pkt_dst,
		 struct net_pkt *pkt_src,
		 size_t length);

/**
 * @brief Clone pkt and its buffer. The cloned packet will be allocated on
 *        the same pool as the original one.
 *
 * @param pkt Original pkt to be cloned
 * @param timeout Timeout to wait for free buffer
 *
 * @return NULL if error, cloned packet otherwise.
 */
struct net_pkt *net_pkt_clone(struct net_pkt *pkt, k_timeout_t timeout);

/**
 * @brief Clone pkt and its buffer. The cloned packet will be allocated on
 *        the RX packet poll.
 *
 * @param pkt Original pkt to be cloned
 * @param timeout Timeout to wait for free buffer
 *
 * @return NULL if error, cloned packet otherwise.
 */
struct net_pkt *net_pkt_rx_clone(struct net_pkt *pkt, k_timeout_t timeout);

/**
 * @brief Clone pkt and increase the refcount of its buffer.
 *
 * @param pkt Original pkt to be shallow cloned
 * @param timeout Timeout to wait for free packet
 *
 * @return NULL if error, cloned packet otherwise.
 */
struct net_pkt *net_pkt_shallow_clone(struct net_pkt *pkt,
				      k_timeout_t timeout);

/**
 * @brief Read some data from a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt    The network packet from where to read some data
 * @param data   The destination buffer where to copy the data
 * @param length The amount of data to copy
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_read(struct net_pkt *pkt, void *data, size_t length);

/**
 * @brief Read a byte (uint8_t) from a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The destination uint8_t where to copy the data
 *
 * @return 0 on success, negative errno code otherwise.
 */
static inline int net_pkt_read_u8(struct net_pkt *pkt, uint8_t *data)
{
	return net_pkt_read(pkt, data, 1);
}

/**
 * @brief Read uint16_t big endian data from a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The destination uint16_t where to copy the data
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_read_be16(struct net_pkt *pkt, uint16_t *data);

/**
 * @brief Read uint16_t little endian data from a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The destination uint16_t where to copy the data
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_read_le16(struct net_pkt *pkt, uint16_t *data);

/**
 * @brief Read uint32_t big endian data from a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The destination uint32_t where to copy the data
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_read_be32(struct net_pkt *pkt, uint32_t *data);

/**
 * @brief Write data into a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt    The network packet where to write
 * @param data   Data to be written
 * @param length Length of the data to be written
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_write(struct net_pkt *pkt, const void *data, size_t length);

/**
 * @brief Write a byte (uint8_t) data to a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The uint8_t value to write
 *
 * @return 0 on success, negative errno code otherwise.
 */
static inline int net_pkt_write_u8(struct net_pkt *pkt, uint8_t data)
{
	return net_pkt_write(pkt, &data, sizeof(uint8_t));
}

/**
 * @brief Write a uint16_t big endian data to a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The uint16_t value in host byte order to write
 *
 * @return 0 on success, negative errno code otherwise.
 */
static inline int net_pkt_write_be16(struct net_pkt *pkt, uint16_t data)
{
	uint16_t data_be16 = htons(data);

	return net_pkt_write(pkt, &data_be16, sizeof(uint16_t));
}

/**
 * @brief Write a uint32_t big endian data to a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The uint32_t value in host byte order to write
 *
 * @return 0 on success, negative errno code otherwise.
 */
static inline int net_pkt_write_be32(struct net_pkt *pkt, uint32_t data)
{
	uint32_t data_be32 = htonl(data);

	return net_pkt_write(pkt, &data_be32, sizeof(uint32_t));
}

/**
 * @brief Write a uint32_t little endian data to a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The uint32_t value in host byte order to write
 *
 * @return 0 on success, negative errno code otherwise.
 */
static inline int net_pkt_write_le32(struct net_pkt *pkt, uint32_t data)
{
	uint32_t data_le32 = sys_cpu_to_le32(data);

	return net_pkt_write(pkt, &data_le32, sizeof(uint32_t));
}

/**
 * @brief Write a uint16_t little endian data to a net_pkt
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt  The network packet from where to read
 * @param data The uint16_t value in host byte order to write
 *
 * @return 0 on success, negative errno code otherwise.
 */
static inline int net_pkt_write_le16(struct net_pkt *pkt, uint16_t data)
{
	uint16_t data_le16 = sys_cpu_to_le16(data);

	return net_pkt_write(pkt, &data_le16, sizeof(uint16_t));
}

/**
 * @brief Get the amount of data which can be read from current cursor position
 *
 * @param pkt Network packet
 *
 * @return Amount of data which can be read from current pkt cursor
 */
size_t net_pkt_remaining_data(struct net_pkt *pkt);

/**
 * @brief Get the total amount of bytes stored in a packet.
 *
 * @param pkt Network packet
 *
 * @return Total amount of bytes stored in a packet.
 */
static inline size_t net_pkt_get_len(struct net_pkt *pkt)
{
	return net_buf_frags_len(pkt->frags);
}

/**
 * @brief Update the overall length of a packet
 *
 * @details Unlike net_pkt_pull() below, this does not take packet cursor
 *          into account. It's mainly a helper dedicated for ipv4 and ipv6
 *          input functions. It shrinks the overall length by given parameter.
 *
 * @param pkt    Network packet
 * @param length The new length of the packet
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_update_length(struct net_pkt *pkt, size_t length);

/**
 * @brief Remove data from the start of the packet.
 *
 * @details net_pkt's cursor should be properly initialized.
 *          Note that net_pkt's cursor is reset by this function.
 *          This functions works in similar way as net_buf_pull(),
 *          but it can handle multiple net_buf fragments.
 *
 * @param pkt    Network packet
 * @param length Number of bytes to be removed
 *
 * @return 0 on success, negative errno code otherwise.
 */
int net_pkt_pull(struct net_pkt *pkt, size_t length);

/**
 * @brief Get the actual offset in the packet from its cursor
 *
 * @param pkt Network packet.
 *
 * @return a valid offset on success, 0 otherwise as there is nothing that
 *         can be done to evaluate the offset.
 */
uint16_t net_pkt_get_current_offset(struct net_pkt *pkt);

/**
 * @brief Check if a data size could fit contiguously
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *
 * @param pkt  Network packet.
 * @param size The size to check for contiguity
 *
 * @return true if that is the case, false otherwise.
 */
bool net_pkt_is_contiguous(struct net_pkt *pkt, size_t size);

/**
 * Get the contiguous buffer space
 *
 * @param pkt Network packet
 *
 * @return The available contiguous buffer space in bytes starting from the
 *         current cursor position. 0 in case of an error.
 */
size_t net_pkt_get_contiguous_len(struct net_pkt *pkt);

/** @cond INTERNAL_HIDDEN */

struct net_pkt_data_access {
#if !defined(CONFIG_NET_HEADERS_ALWAYS_CONTIGUOUS)
	void *data;
#endif
	const size_t size;
};

#if defined(CONFIG_NET_HEADERS_ALWAYS_CONTIGUOUS)
#define NET_PKT_DATA_ACCESS_DEFINE(_name, _type)		\
	struct net_pkt_data_access _name = {			\
		.size = sizeof(_type),				\
	}

#define NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(_name, _type)	\
	NET_PKT_DATA_ACCESS_DEFINE(_name, _type)

#else
#define NET_PKT_DATA_ACCESS_DEFINE(_name, _type)		\
	_type _hdr_##_name;					\
	struct net_pkt_data_access _name = {			\
		.data = &_hdr_##_name,				\
		.size = sizeof(_type),				\
	}

#define NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(_name, _type)	\
	struct net_pkt_data_access _name = {			\
		.data = NULL,					\
		.size = sizeof(_type),				\
	}

#endif /* CONFIG_NET_HEADERS_ALWAYS_CONTIGUOUS */

/** @endcond */

/**
 * @brief Get data from a network packet in a contiguous way
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip. Unlike other functions,
 *          cursor position will not be updated after the operation.
 *
 * @param pkt    The network packet from where to get the data.
 * @param access A pointer to a valid net_pkt_data_access describing the
 *        data to get in a contiguous way.
 *
 * @return a pointer to the requested contiguous data, NULL otherwise.
 */
void *net_pkt_get_data(struct net_pkt *pkt,
		       struct net_pkt_data_access *access);

/**
 * @brief Set contiguous data into a network packet
 *
 * @details net_pkt's cursor should be properly initialized and,
 *          if needed, positioned using net_pkt_skip.
 *          Cursor position will be updated after the operation.
 *
 * @param pkt    The network packet to where the data should be set.
 * @param access A pointer to a valid net_pkt_data_access describing the
 *        data to set.
 *
 * @return 0 on success, a negative errno otherwise.
 */
int net_pkt_set_data(struct net_pkt *pkt,
		     struct net_pkt_data_access *access);

/**
 * Acknowledge previously contiguous data taken from a network packet
 * Packet needs to be set to overwrite mode.
 */
static inline int net_pkt_acknowledge_data(struct net_pkt *pkt,
					   struct net_pkt_data_access *access)
{
	return net_pkt_skip(pkt, access->size);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_PKT_H_ */
