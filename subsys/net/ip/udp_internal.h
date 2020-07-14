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
#if defined(CONFIG_NET_NATIVE_UDP)
int net_udp_create(struct net_pkt *pkt, uint16_t src_port, uint16_t dst_port);
#else
static inline int net_udp_create(struct net_pkt *pkt,
				 uint16_t src_port, uint16_t dst_port)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(src_port);
	ARG_UNUSED(dst_port);

	return 0;
}
#endif

/**
 * @brief Finalize UDP packet
 *
 * Note: calculates final length and setting up the checksum.
 *
 * @param pkt Network packet
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_UDP)
int net_udp_finalize(struct net_pkt *pkt);
#else
static inline int net_udp_finalize(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}
#endif

/**
 * @brief Get pointer to UDP header in net_pkt
 *
 * @param pkt Network packet
 * @param udp_access Helper variable for accessing UDP header
 *
 * @return UDP header on success, NULL on error
 */
#if defined(CONFIG_NET_NATIVE_UDP)
struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access);
#else
static inline
struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(udp_access);

	return NULL;
}
#endif

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
int net_udp_register(uint8_t family,
		     const struct sockaddr *remote_addr,
		     const struct sockaddr *local_addr,
		     uint16_t remote_port,
		     uint16_t local_port,
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
