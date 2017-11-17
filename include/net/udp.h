/** @file
 @brief UDP utility functions
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UDP_H
#define __UDP_H

#include <zephyr/types.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UDP library
 * @defgroup udp UDP Library
 * @ingroup networking
 * @{
 */

#if defined(CONFIG_NET_UDP)

/**
 * @brief Get UDP packet header data from net_pkt.
 *
 * @details The values in the returned header are in network byte order.
 * Note that you must access the UDP header values by the returned pointer,
 * the hdr parameter is just a placeholder for the header data and it might
 * not contain anything if the header fits properly in the first fragment of
 * the network packet.
 *
 * @param pkt Network packet
 * @param hdr Where to place the header if it does not fit in first fragment
 * of the network packet. This might not be populated if UDP header fits in
 * net_buf fragment.
 *
 * @return Return pointer to header or NULL if something went wrong.
 *         Always use the returned pointer to access the UDP header.
 */
struct net_udp_hdr *net_udp_get_hdr(struct net_pkt *pkt,
				    struct net_udp_hdr *hdr);

/**
 * @brief Set UDP packet header data in net_pkt.
 *
 * @details The values in the header must be in network byte order.
 * This function is normally called after a call to net_udp_get_hdr().
 * The hdr parameter value should be the same that is returned by function
 * net_udp_get_hdr() call. Note that if the UDP header fits in first net_pkt
 * fragment, then this function will not do anything as your hdr parameter
 * was pointing directly to net_pkt.
 *
 * @param pkt Network packet
 * @param hdr Header data pointer that was returned by net_udp_get_hdr().
 *
 * @return Return pointer to header or NULL if something went wrong.
 */
struct net_udp_hdr *net_udp_set_hdr(struct net_pkt *pkt,
				    struct net_udp_hdr *hdr);

#else

static inline struct net_udp_hdr *net_udp_get_hdr(struct net_pkt *pkt,
						  struct net_udp_hdr *hdr)
{
	return NULL;
}

static inline struct net_udp_hdr *net_udp_set_hdr(struct net_pkt *pkt,
						  struct net_udp_hdr *hdr)
{
	return NULL;
}

#endif /* CONFIG_NET_UDP */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __UDP_H */
