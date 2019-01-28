/** @file
 @brief UDP data handler

 This is not to be included by the application and is only used by
 core IP stack.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UDP_INTERNAL_H
#define __UDP_INTERNAL_H

#include <zephyr/types.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_context.h>

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_UDP)

/**
 * @brief Set UDP checksum in network packet.
 *
 * @param pkt Network packet
 * @param frag Fragment where to start calculating the offset.
 * Typically this is set to pkt->frags by the caller.
 *
 * @return Return the actual fragment where the checksum was written.
 */
struct net_buf *net_udp_set_chksum(struct net_pkt *pkt, struct net_buf *frag);

/**
 * @brief Insert UDP packet into net_pkt after specific offset.
 *
 * @param pkt Network packet
 * @param offset Offset where to insert (typically after IP header)
 * @param src_port Destination port in network byte order.
 * @param dst_port Destination port in network byte order.
 *
 * @return Return network packet that contains the UDP packet or NULL if
 * there is an failure.
 */
struct net_pkt *net_udp_insert(struct net_pkt *pkt,
			       u16_t offset,
			       u16_t src_port,
			       u16_t dst_port);

/**
 * @brief Create UDP packet into net_pkt
 *
 * Note: pkt's cursor should be set a the right position.
 *       (i.e. after IP header)
 *
 * @param pkt Network packet
 * @param src_port Destination port in network byte order.
 * @param dst_port Destination port in network byte order.
 *
 * @return 0 on success, negative errno otherwise.
 */
int net_udp_create(struct net_pkt *pkt, u16_t src_port, u16_t dst_port);

/**
 * @brief Finalize UDP packet
 *
 * Note: calculates final length and setting up the checksum.
 *
 * @param pkt Network packet
 *
 * @return 0 on success, negative errno otherwise.
 */
int net_udp_finalize(struct net_pkt *pkt);

struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access);

#else
#define net_udp_insert(pkt, offset, src_port, dst_port) (pkt)
#define net_udp_set_chksum(pkt, frag) NULL

static inline int net_udp_create(struct net_pkt *pkt,
				 u16_t src_port, u16_t dst_port)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(src_port);
	ARG_UNUSED(dst_port);

	return 0;
}

static inline int net_udp_finalize(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline
struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(udp_access);

	return NULL;
}

#endif /* CONFIG_NET_UDP */

/**
 * @brief Register a callback to be called when UDP packet
 * is received corresponding to received packet.
 *
 * @param family Protocol family
 * @param remote_addr Remote address of the connection end point.
 * @param local_addr Local address of the connection end point.
 * @param remote_port Remote port of the connection end point.
 * @param local_port Local port of the connection end point.
 * @param cb Callback to be called
 * @param user_data User data supplied by caller.
 * @param handle UDP handle that can be used when unregistering
 *
 * @return Return 0 if the registration succeed, <0 otherwise.
 */
int net_udp_register(u8_t family,
		     const struct sockaddr *remote_addr,
		     const struct sockaddr *local_addr,
		     u16_t remote_port,
		     u16_t local_port,
		     net_conn_cb_t cb,
		     void *user_data,
		     struct net_conn_handle **handle);

/**
 * @brief Unregister UDP handler.
 *
 * @param handle Handle from registering.
 *
 * @return Return 0 if the unregistration succeed, <0 otherwise.
 */
int net_udp_unregister(struct net_conn_handle *handle);

#ifdef __cplusplus
}
#endif

#endif /* __UDP_INTERNAL_H */
