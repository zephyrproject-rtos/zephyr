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

#include <stdint.h>

#include <net/net_ip.h>
#include <net/nbuf.h>
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
 * except for destination option. This information is stored in bitfield variable.
 * The order of the bitmap is the order recommended in RFC 2460.
 */
#define NET_IPV6_EXT_HDR_BITMAP_HBHO	0x01
#define NET_IPV6_EXT_HDR_BITMAP_DESTO1	0x02
#define NET_IPV6_EXT_HDR_BITMAP_ROUTING	0x04
#define NET_IPV6_EXT_HDR_BITMAP_FRAG	0x08
#define NET_IPV6_EXT_HDR_BITMAP_AH	0x10
#define NET_IPV6_EXT_HDR_BITMAP_ESP	0x20
#define NET_IPV6_EXT_HDR_BITMAP_DESTO2	0x40

/**
 * @brief Destination and Hop By Hop extension headers option types
 */
#define NET_IPV6_EXT_HDR_OPT_PAD1  0
#define NET_IPV6_EXT_HDR_OPT_PADN  1
#define NET_IPV6_EXT_HDR_OPT_RPL   0x63

/* State of the neighbor */
enum net_nbr_state {
	NET_NBR_INCOMPLETE,
	NET_NBR_REACHABLE,
	NET_NBR_STALE,
	NET_NBR_DELAY,
	NET_NBR_PROBE,
};

/**
 * @brief IPv6 neighbor information.
 */
struct net_ipv6_nbr_data {
	/** Any pending buffer waiting ND to finish. */
	struct net_buf *pending;

	/** IPv6 address. */
	struct in6_addr addr;

	/** Reachable timer. */
	struct k_delayed_work reachable;

	/** Neighbor Solicitation timer for DAD */
	struct k_delayed_work send_ns;

	/** State of the neighbor discovery */
	enum net_nbr_state state;

	/** Link metric for the neighbor */
	uint16_t link_metric;

	/** How many times we have sent NS */
	uint8_t ns_count;

	/** Is the neighbor a router */
	bool is_router;
};

static inline struct net_ipv6_nbr_data *net_ipv6_nbr_data(struct net_nbr *nbr)
{
	return (struct net_ipv6_nbr_data *)nbr->data;
}

/**
 * @brief Return IPv6 neighbor according to ll index.
 *
 * @param idx Neighbor index in link layer table.
 *
 * @return Return IPv6 neighbor information.
 */
struct net_ipv6_nbr_data *net_ipv6_get_nbr_by_index(uint8_t idx);

#if defined(CONFIG_NET_IPV6_DAD)
int net_ipv6_start_dad(struct net_if *iface, struct net_if_addr *ifaddr);
#endif

int net_ipv6_send_ns(struct net_if *iface, struct net_buf *pending,
		     struct in6_addr *src, struct in6_addr *dst,
		     struct in6_addr *tgt, bool is_my_address);

int net_ipv6_send_rs(struct net_if *iface);
int net_ipv6_start_rs(struct net_if *iface);

int net_ipv6_send_na(struct net_if *iface, struct in6_addr *src,
		     struct in6_addr *dst, struct in6_addr *tgt,
		     uint8_t flags);

/**
 * @brief Create IPv6 packet in provided net_buf.
 *
 * @param buf Network buffer
 * @param reserve Link layer reserve
 * @param src Source IPv6 address
 * @param dst Destination IPv6 address
 * @param iface Network interface
 * @param next_header Protocol type of the next header after IPv6 header.
 *
 * @return Return network buffer that contains the IPv6 packet.
 */
struct net_buf *net_ipv6_create_raw(struct net_buf *buf,
				    uint16_t reserve,
				    const struct in6_addr *src,
				    const struct in6_addr *dst,
				    struct net_if *iface,
				    uint8_t next_header);

/**
 * @brief Create IPv6 packet in provided net_buf.
 *
 * @param context Network context for a connection
 * @param buf Network buffer
 * @param src_addr Source address, or NULL to choose a default from context
 * @param dst_addr Destination IPv6 address
 *
 * @return Return network buffer that contains the IPv6 packet.
 */
struct net_buf *net_ipv6_create(struct net_context *context,
				struct net_buf *buf,
				const struct in6_addr *src_addr,
				const struct in6_addr *dst_addr);

/**
 * @brief Finalize IPv6 packet. It should be called right before
 * sending the packet and after all the data has been added into
 * the packet. This function will set the length of the
 * packet and calculate the higher protocol checksum if needed.
 *
 * @param buf Network buffer
 * @param next_header Protocol type of the next header after IPv6 header.
 *
 * @return Return network buffer that contains the IPv6 packet.
 */
struct net_buf *net_ipv6_finalize_raw(struct net_buf *buf,
				      uint8_t next_header);

/**
 * @brief Finalize IPv6 packet. It should be called right before
 * sending the packet and after all the data has been added into
 * the packet. This function will set the length of the
 * packet and calculate the higher protocol checksum if needed.
 *
 * @param context Network context for a connection
 * @param buf Network buffer
 *
 * @return Return network buffer that contains the IPv6 packet.
 */
struct net_buf *net_ipv6_finalize(struct net_context *context,
				  struct net_buf *buf);

#if defined(CONFIG_NET_IPV6_ND)
/**
 * @brief Make sure the link layer address is set according to
 * destination address. If the ll address is not yet known, then
 * start neighbor discovery to find it out. If ND needs to be done
 * then the returned packet is the Neighbor Solicitation message
 * and the original message is sent after Neighbor Advertisement
 * message is received.
 *
 * @param buf Network buffer
 *
 * @return Return network buffer to be sent.
 */
struct net_buf *net_ipv6_prepare_for_send(struct net_buf *buf);

/**
 * @brief Look for a neighbour from it's address on an iface
 *
 * @param iface A valid pointer on a network interface
 * @param addr The IPv6 address to match
 *
 * @return A valid pointer on a neighbour on success, NULL otherwise
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
 * @return A valid pointer on a neighbour on success, NULL otherwise
 */
struct net_nbr *net_ipv6_get_nbr(struct net_if *iface, uint8_t idx);

/**
 * @brief Look for a neighbour from it's link local address index
 *
 * @param iface Network interface to match. If NULL, then use
 * whatever interface there is configured for the neighbor address.
 * @param idx Index of the link layer address in the address array
 *
 * @return A valid pointer on a neighbour on success, NULL otherwise
 */
struct in6_addr *net_ipv6_nbr_lookup_by_index(struct net_if *iface,
					      uint8_t idx);

/**
 * @brief Add a neighbour to neighbor cache
 *
 * @param iface A valid pointer on a network interface
 * @param addr Neighbor IPv6 address
 * @param lladdr Neighbor link layer address
 * @param is_router Set to true if the neighbor is a router, false
 * otherwise
 * @param state Initial state of the neighbor entry in the cache
 *
 * @return A valid pointer on a neighbour on success, NULL otherwise
 */
struct net_nbr *net_ipv6_nbr_add(struct net_if *iface,
				 struct in6_addr *addr,
				 struct net_linkaddr *lladdr,
				 bool is_router,
				 enum net_nbr_state state);

/**
 * @brief Remove a neighbour from neighbor cache.
 *
 * @param iface A valid pointer on a network interface
 * @param addr Neighbor IPv6 address
 *
 * @return True if neighbor could be removed, False otherwise
 */
bool net_ipv6_nbr_rm(struct net_if *iface, struct in6_addr *addr);

/**
 * @brief Set the neighbor reachable timer.
 *
 * @param iface A valid pointer on a network interface
 * @param nbr Neighbor struct pointer
 */
void net_ipv6_nbr_set_reachable_timer(struct net_if *iface,
				      struct net_nbr *nbr);

/**
 * @typedef net_nbr_cb_t
 * @brief Callback used while iterating over neighbors.
 *
 * @param nbr A valid pointer on current neighbor.
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*net_nbr_cb_t)(struct net_nbr *nbr, void *user_data);

/**
 * @brief Go through all the neighbors and call callback for each of them.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 */
void net_ipv6_nbr_foreach(net_nbr_cb_t cb, void *user_data);

#else /* CONFIG_NET_IPV6_ND */
static inline struct net_buf *net_ipv6_prepare_for_send(struct net_buf *buf)
{
	return buf;
}

static inline struct net_nbr *net_ipv6_nbr_lookup(struct net_if *iface,
						  struct in6_addr *addr)
{
	return NULL;
}

static inline
struct in6_addr *net_ipv6_nbr_lookup_by_index(struct net_if *iface,
					      uint8_t idx)
{
	return NULL;
}

static inline struct net_nbr *net_ipv6_nbr_add(struct net_if *iface,
					       struct in6_addr *addr,
					       struct net_linkaddr *lladdr,
					       bool is_router,
					       enum net_nbr_state state)
{
	return NULL;
}

static inline void net_ipv6_nbr_set_reachable_timer(struct net_if *iface,
						    struct net_nbr *nbr)
{
}
#endif

#if defined(CONFIG_NET_IPV6)
void net_ipv6_init(void);
#else
#define net_ipv6_init(...)
#endif

#endif /* __IPV6_H */
