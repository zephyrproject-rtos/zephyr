/** @file
 * @brief Packet Socket related functions
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PACKET_SOCKET_H
#define __PACKET_SOCKET_H

#include <zephyr/types.h>

/**
 * @brief Called by net_core.c when a network packet is received.
 *
 * @param pkt Network packet
 *
 */
#if defined(CONFIG_NET_SOCKETS_PACKET)
void net_packet_socket_input(struct net_pkt *pkt, uint16_t proto, enum net_sock_type type);
#else
static inline void net_packet_socket_input(struct net_pkt *pkt,
					   uint16_t proto,
					   enum net_sock_type type)
{
}
#endif

#endif /* __PACKET_SOCKET_H */
