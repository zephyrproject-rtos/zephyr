/** @file
 * @brief TCP utility functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_TCP_H_
#define ZEPHYR_INCLUDE_NET_TCP_H_

#include <zephyr/types.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_context.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_TCP)
/**
 * @brief Get TCP packet header data from net_pkt. The array values are in
 * network byte order and other values are in host byte order.
 * Note that you must access the TCP header values by the returned pointer,
 * the hdr parameter is just a placeholder for the header data and it might
 * not contain anything if the header fits properly in the first fragment of
 * the network packet.
 *
 * @param pkt Network packet
 * @param hdr Where to place the header if it does not fit in first fragment
 * of the network packet. This might not be populated if TCP header fits in
 * net_buf fragment.
 *
 * @return Return pointer to header or NULL if something went wrong.
 *         Always use the returned pointer to access the TCP header.
 */
struct net_tcp_hdr *net_tcp_get_hdr(struct net_pkt *pkt,
				    struct net_tcp_hdr *hdr);

/**
 * @brief Set TCP packet header data in net_pkt.
 *
 * @details  The values in the header must be in network byte order.
 * This function is normally called after a call to net_tcp_get_hdr().
 * The hdr parameter value should be the same that is returned by function
 * net_tcp_get_hdr() call. Note that if the TCP header fits in first net_pkt
 * fragment, then this function will not do anything as the returned value
 * was pointing directly to net_pkt.
 *
 * @param pkt Network packet
 * @param hdr Header data pointer that was returned by net_tcp_get_hdr().
 *
 * @return Return hdr or NULL if error
 */
struct net_tcp_hdr *net_tcp_set_hdr(struct net_pkt *pkt,
				    struct net_tcp_hdr *hdr);

#endif /* CONFIG_NET_TCP */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_TCP_H_ */
