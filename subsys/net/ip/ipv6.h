/** @file
 @brief IPv6 data handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IPV6_H
#define __IPV6_H

#include <zephyr/types.h>

#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_context.h>

#include "icmpv6.h"
#include "nbr.h"

#define NET_IPV6_ND_HOP_LIMIT 255
#define NET_IPV6_ND_INFINITE_LIFETIME 0xFFFFFFFF

#define NET_IPV6_DEFAULT_PREFIX_LEN 64

#define NET_MAX_RS_COUNT 3

/**
 * @brief Bitmaps for IPv6 extension header processing
 *
 * When processing extension headers, we record which one we have seen.
 * This is done as the network packet cannot have twice the same header,
 * except for destination option.
 * This information is stored in bitfield variable.
 * The order of the bitmap is the order recommended in RFC 2460.
 */
#define NET_IPV6_EXT_HDR_BITMAP_HBHO   0x01
#define NET_IPV6_EXT_HDR_BITMAP_DESTO1 0x02
#define NET_IPV6_EXT_HDR_BITMAP_ROUTING        0x04
#define NET_IPV6_EXT_HDR_BITMAP_FRAG   0x08
#define NET_IPV6_EXT_HDR_BITMAP_AH     0x10
#define NET_IPV6_EXT_HDR_BITMAP_ESP    0x20
#define NET_IPV6_EXT_HDR_BITMAP_DESTO2 0x40

/**
 * @brief Destination and Hop By Hop extension headers option types
 */
#define NET_IPV6_EXT_HDR_OPT_PAD1  0
#define NET_IPV6_EXT_HDR_OPT_PADN  1
#define NET_IPV6_EXT_HDR_OPT_RPL   0x63

/**
 * @brief Multicast Listener Record v2 record types.
 */
#define NET_IPV6_MLDv2_MODE_IS_INCLUDE        1
#define NET_IPV6_MLDv2_MODE_IS_EXCLUDE        2
#define NET_IPV6_MLDv2_CHANGE_TO_INCLUDE_MODE 3
#define NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE 4
#define NET_IPV6_MLDv2_ALLOW_NEW_SOURCES      5
#define NET_IPV6_MLDv2_BLOCK_OLD_SOURCES      6

/* State of the neighbor */
enum net_ipv6_nbr_state {
	NET_IPV6_NBR_STATE_INCOMPLETE,
	NET_IPV6_NBR_STATE_REACHABLE,
	NET_IPV6_NBR_STATE_STALE,
	NET_IPV6_NBR_STATE_DELAY,
	NET_IPV6_NBR_STATE_PROBE,
	NET_IPV6_NBR_STATE_STATIC,
};

const char *net_ipv6_nbr_state2str(enum net_ipv6_nbr_state state);

/**
 * @brief IPv6 neighbor information.
 */
struct net_ipv6_nbr_data {
	/** Any pending packet waiting ND to finish. */
	struct net_pkt *pending;

	/** IPv6 address. */
	struct in6_addr addr;

	/** Reachable timer. */
	s64_t reachable;

	/** Reachable timeout */
	s32_t reachable_timeout;

	/** Neighbor Solicitation reply timer */
	s64_t send_ns;

	/** State of the neighbor discovery */
	enum net_ipv6_nbr_state state;

	/** Link metric for the neighbor */
	u16_t link_metric;

	/** How many times we have sent NS */
	u8_t ns_count;

	/** Is the neighbor a router */
	bool is_router;
};

static inline struct net_ipv6_nbr_data *net_ipv6_nbr_data(struct net_nbr *nbr)
{
	return (struct net_ipv6_nbr_data *)nbr->data;
}

#if defined(CONFIG_NET_IPV6_DAD)
int net_ipv6_start_dad(struct net_if *iface, struct net_if_addr *ifaddr);
#endif

int net_ipv6_send_ns(struct net_if *iface, struct net_pkt *pending,
		     struct in6_addr *src, struct in6_addr *dst,
		     struct in6_addr *tgt, bool is_my_address);

int net_ipv6_send_rs(struct net_if *iface);
int net_ipv6_start_rs(struct net_if *iface);

int net_ipv6_send_na(struct net_if *iface, const struct in6_addr *src,
		     const struct in6_addr *dst, const struct in6_addr *tgt,
		     u8_t flags);

/**
 * @brief Create IPv6 packet in provided net_pkt.
 *
 * @param pkt Network packet
 * @param src Source IPv6 address
 * @param dst Destination IPv6 address
 * @param iface Network interface
 * @param next_header_proto Protocol type of the next header after IPv6 header.
 *
 * @return Returns the input pkt network packet (which now contains IPv6
 * header) or NULL if network packet could not be allocated.
 */
struct net_pkt *net_ipv6_create(struct net_pkt *pkt,
				const struct in6_addr *src,
				const struct in6_addr *dst,
				struct net_if *iface,
				u8_t next_header_proto);

/**
 * @brief Finalize IPv6 packet. It should be called right before
 * sending the packet and after all the data has been added into
 * the packet. This function will set the length of the
 * packet and calculate the higher protocol checksum if needed.
 *
 * @param pkt Network packet
 * @param next_header_proto Protocol type of the next header after IPv6 header.
 *
 * @return Return 0 on Success, < 0 on Failure.
 */
int net_ipv6_finalize(struct net_pkt *pkt, u8_t next_header_proto);

#if defined(CONFIG_NET_IPV6_MLD)
/**
 * @brief Join a given multicast group.
 *
 * @param iface Network interface where join message is sent
 * @param addr Multicast group to join
 *
 * @return Return 0 if joining was done, <0 otherwise.
 */
int net_ipv6_mld_join(struct net_if *iface, const struct in6_addr *addr);

/**
 * @brief Leave a given multicast group.
 *
 * @param iface Network interface where leave message is sent
 * @param addr Multicast group to leave
 *
 * @return Return 0 if leaving is done, <0 otherwise.
 */
int net_ipv6_mld_leave(struct net_if *iface, const struct in6_addr *addr);
#else
#define net_ipv6_mld_join(...)
#define net_ipv6_mld_leave(...)
#endif /* CONFIG_NET_IPV6_MLD */

/**
 * @typedef net_nbr_cb_t
 * @brief Callback used while iterating over neighbors.
 *
 * @param nbr A valid pointer on current neighbor.
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*net_nbr_cb_t)(struct net_nbr *nbr, void *user_data);

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
/**
 * @brief Make sure the link layer address is set according to
 * destination address. If the ll address is not yet known, then
 * start neighbor discovery to find it out. If ND needs to be done
 * then the returned packet is the Neighbor Solicitation message
 * and the original message is sent after Neighbor Advertisement
 * message is received.
 *
 * @param pkt Network packet
 *
 * @return Return network packet to be sent.
 */
struct net_pkt *net_ipv6_prepare_for_send(struct net_pkt *pkt);

/**
 * @brief Look for a neighbor from it's address on an iface
 *
 * @param iface A valid pointer on a network interface
 * @param addr The IPv6 address to match
 *
 * @return A valid pointer on a neighbor on success, NULL otherwise
 */
struct net_nbr *net_ipv6_nbr_lookup(struct net_if *iface,
				    struct in6_addr *addr);

/**
 * @brief Get neighbor from its index.
 *
 * @param iface Network interface to match. If NULL, then use
 * whatever interface there is configured for the neighbor address.
 * @param idx Index of the link layer address in the address array
 *
 * @return A valid pointer on a neighbor on success, NULL otherwise
 */
struct net_nbr *net_ipv6_get_nbr(struct net_if *iface, u8_t idx);

/**
 * @brief Look for a neighbor from it's link local address index
 *
 * @param iface Network interface to match. If NULL, then use
 * whatever interface there is configured for the neighbor address.
 * @param idx Index of the link layer address in the address array
 *
 * @return A valid pointer on a neighbor on success, NULL otherwise
 */
struct in6_addr *net_ipv6_nbr_lookup_by_index(struct net_if *iface,
					      u8_t idx);

/**
 * @brief Add a neighbor to neighbor cache
 *
 * Add a neighbor to the cache after performing a lookup and in case
 * there exists an entry in the cache update its state and lladdr.
 *
 * @param iface A valid pointer on a network interface
 * @param addr Neighbor IPv6 address
 * @param lladdr Neighbor link layer address
 * @param is_router Set to true if the neighbor is a router, false
 * otherwise
 * @param state Initial state of the neighbor entry in the cache
 *
 * @return A valid pointer on a neighbor on success, NULL otherwise
 */
struct net_nbr *net_ipv6_nbr_add(struct net_if *iface,
				 struct in6_addr *addr,
				 struct net_linkaddr *lladdr,
				 bool is_router,
				 enum net_ipv6_nbr_state state);

/**
 * @brief Remove a neighbor from neighbor cache.
 *
 * @param iface A valid pointer on a network interface
 * @param addr Neighbor IPv6 address
 *
 * @return True if neighbor could be removed, False otherwise
 */
bool net_ipv6_nbr_rm(struct net_if *iface, struct in6_addr *addr);

/**
 * @brief Go through all the neighbors and call callback for each of them.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 */
void net_ipv6_nbr_foreach(net_nbr_cb_t cb, void *user_data);

#else /* CONFIG_NET_IPV6_NBR_CACHE */
static inline struct net_pkt *net_ipv6_prepare_for_send(struct net_pkt *pkt)
{
	return pkt;
}

static inline struct net_nbr *net_ipv6_nbr_lookup(struct net_if *iface,
						  struct in6_addr *addr)
{
	return NULL;
}

static inline
struct in6_addr *net_ipv6_nbr_lookup_by_index(struct net_if *iface,
					      u8_t idx)
{
	return NULL;
}

static inline struct net_nbr *net_ipv6_nbr_add(struct net_if *iface,
					       struct in6_addr *addr,
					       struct net_linkaddr *lladdr,
					       bool is_router,
					       enum net_ipv6_nbr_state state)
{
	return NULL;
}

static inline bool net_ipv6_nbr_rm(struct net_if *iface, struct in6_addr *addr)
{
	return true;
}

static inline void net_ipv6_nbr_foreach(net_nbr_cb_t cb, void *user_data)
{
	return;
}
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

#if defined(CONFIG_NET_IPV6_ND)
/**
 * @brief Set the neighbor reachable timer.
 *
 * @param iface A valid pointer on a network interface
 * @param nbr Neighbor struct pointer
 */
void net_ipv6_nbr_set_reachable_timer(struct net_if *iface,
				      struct net_nbr *nbr);

#else /* CONFIG_NET_IPV6_ND */
static inline void net_ipv6_nbr_set_reachable_timer(struct net_if *iface,
						    struct net_nbr *nbr)
{
}
#endif

#if defined(CONFIG_NET_IPV6_FRAGMENT)
/* We do not have to accept larger than 1500 byte IPv6 packet (RFC 2460 ch 5).
 * This means that we should receive everything within first two fragments.
 * The first one being 1280 bytes and the second one 220 bytes.
 */
#if !defined(NET_IPV6_FRAGMENTS_MAX_PKT)
#define NET_IPV6_FRAGMENTS_MAX_PKT 2
#endif

/** Store pending IPv6 fragment information that is needed for reassembly. */
struct net_ipv6_reassembly {
	/** IPv6 source address of the fragment */
	struct in6_addr src;

	/** IPv6 destination address of the fragment */
	struct in6_addr dst;

	/**
	 * Timeout for cancelling the reassembly. The timer is used
	 * also to detect if this reassembly slot is used or not.
	 */
	struct k_delayed_work timer;

	/** Pointers to pending fragments */
	struct net_pkt *pkt[NET_IPV6_FRAGMENTS_MAX_PKT];

	/** IPv6 fragment identification */
	u32_t id;
};

/**
 * @typedef net_ipv6_frag_cb_t
 * @brief Callback used while iterating over pending IPv6 fragments.
 *
 * @param reass IPv6 fragment reassembly struct
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*net_ipv6_frag_cb_t)(struct net_ipv6_reassembly *reass,
				   void *user_data);

/**
 * @brief Go through all the currently pending IPv6 fragments.
 *
 * @param cb Callback to call for each pending IPv6 fragment.
 * @param user_data User specified data or NULL.
 */
void net_ipv6_frag_foreach(net_ipv6_frag_cb_t cb, void *user_data);

/**
 * @brief Find the last IPv6 extension header in the network packet.
 *
 * @param pkt Network head packet.
 * @param next_hdr_idx Where is the index to next header field that points
 * to last header. This is returned to caller.
 * @param last_hdr_idx Where is the last header field in the packet.
 * This is returned to caller.
 *
 * @return Return 0 if ok or <0 if the packet is malformed.
 */
int net_ipv6_find_last_ext_hdr(struct net_pkt *pkt, u16_t *next_hdr_idx,
			       u16_t *last_hdr_idx);

/**
 * @brief Handles IPv6 fragmented packets.
 *
 * @param pkt Network head packet.
 * @param frag Network packet fragment
 * @param total_len Total length of the packet
 * @param offset Start of fragment header
 * @param loc End of fragment header, this is returned to the caller
 * @param nexthdr IPv6 next header after fragment header part
 *
 * @return Return verdict about the packet
 */
enum net_verdict net_ipv6_handle_fragment_hdr(struct net_pkt *pkt,
					      struct net_buf *frag,
					      int total_len,
					      u16_t buf_offset,
					      u16_t *loc,
					      u8_t nexthdr);
#endif /* CONFIG_NET_IPV6_FRAGMENT */

#if defined(CONFIG_NET_IPV6)
void net_ipv6_init(void);
void net_ipv6_nbr_init(void);
#if defined(CONFIG_NET_IPV6_MLD)
void net_ipv6_mld_init(void);
#else
#define net_ipv6_mld_init(...)
#endif
#else
#define net_ipv6_init(...)
#define net_ipv6_nbr_init(...)
#endif

#endif /* __IPV6_H */
