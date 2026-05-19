/** @file
 * @brief Packet Sockets related functions
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sockets_raw, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/ethernet.h>

#include "connection.h"
#include "packet_socket.h"

void net_packet_socket_input(struct net_pkt *pkt,
					 uint16_t proto,
					 enum net_sock_type type)
{
	net_sa_family_t orig_family;

	orig_family = net_pkt_family(pkt);

	net_pkt_set_family(pkt, NET_AF_PACKET);

	net_conn_packet_input(pkt, proto, type);

	net_pkt_set_family(pkt, orig_family);
}
