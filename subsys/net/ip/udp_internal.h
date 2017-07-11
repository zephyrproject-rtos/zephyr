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
 * @brief Append UDP packet into net_pkt
 *
 * @param pkt Network packet
 * @param src_port Source port in host byte order.
 * @param dst_port Destination port in host byte order.
 *
 * @return Return network packet that contains the UDP packet.
 */
struct net_pkt *net_udp_append_raw(struct net_pkt *pkt,
				   u16_t src_port,
				   u16_t dst_port);

/**
 * @brief Insert UDP packet into net_pkt chain. The UDP header is added
 * right after the IP header which is after first fragment.
 *
 * @param pkt Network packet
 * @param offset Offset where to insert (typically after IP header)
 * @param src_port Source port in host byte order.
 * @param dst_port Destination port in host byte order.
 *
 * @return Return network packet that contains the UDP packet.
 */
struct net_pkt *net_udp_insert_raw(struct net_pkt *pkt,
				   u16_t offset,
				   u16_t src_port,
				   u16_t dst_port);

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
 * @brief Get UDP checksum from network packet.
 *
 * @param pkt Network packet
 * @param frag Fragment where to start calculating the offset.
 * Typically this is set to pkt->frags by the caller.
 *
 * @return Return the checksum in host byte order.
 */
u16_t net_udp_get_chksum(struct net_pkt *pkt, struct net_buf *frag);

/**
 * @brief Append UDP packet into net_pkt
 *
 * @param context Network context for a connection
 * @param pkt Network packet
 * @param port Destination port in network byte order.
 *
 * @return Return network packet that contains the UDP packet.
 */
struct net_pkt *net_udp_append(struct net_context *context,
			       struct net_pkt *pkt,
			       u16_t port);

/**
 * @brief Insert UDP packet into net_pkt after specific offset.
 *
 * @param context Network context for a connection
 * @param pkt Network packet
 * @param offset Offset where to insert (typically after IP header)
 * @param port Destination port in network byte order.
 *
 * @return Return network packet that contains the UDP packet or NULL if
 * there is an failure.
 */
struct net_pkt *net_udp_insert(struct net_context *context,
			       struct net_pkt *pkt,
			       u16_t offset,
			       u16_t port);

#else
#define net_udp_append_raw(pkt, src_port, dst_port) (pkt)
#define net_udp_append(context, pkt, port) (pkt)
#define net_udp_insert_raw(pkt, offset, src_port, dst_port) (pkt)
#define net_udp_insert(context, pkt, offset, port) (pkt)
#define net_udp_get_chksum(pkt, frag) (0)
#define net_udp_set_chksum(pkt, frag) NULL
#endif /* CONFIG_NET_UDP */

/**
 * @brief Register a callback to be called when UDP packet
 * is received corresponding to received packet.
 *
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
int net_udp_register(const struct sockaddr *remote_addr,
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

#if defined(CONFIG_NET_UDP)
void net_udp_init(void);
#else
#define net_udp_init(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UDP_INTERNAL_H */
