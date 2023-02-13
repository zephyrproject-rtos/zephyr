/** @file
 * @brief CANBUS Socket related functions
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CAN_SOCKET_H
#define __CAN_SOCKET_H

#include <zephyr/types.h>

/**
 * @brief Called by net_core.c when a CANBUS packet is received.
 *
 * @param pkt Network packet
 *
 * @return NET_OK if the packet was consumed, NET_DROP if
 * the packet parsing failed and the caller should handle
 * the received packet.
 */
#if defined(CONFIG_NET_SOCKETS_CAN)
enum net_verdict net_canbus_socket_input(struct net_pkt *pkt);
#else
static inline enum net_verdict net_canbus_socket_input(struct net_pkt *pkt)
{
	return NET_CONTINUE;
}
#endif

#endif /* __CAN_SOCKET_H */
