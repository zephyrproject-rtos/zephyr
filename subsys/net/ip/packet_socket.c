/** @file
 * @brief Packet Sockets related functions
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_sockets_raw, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <errno.h>
#include <net/net_pkt.h>
#include <net/net_context.h>
#include <net/ethernet.h>

#include "connection.h"
#include "packet_socket.h"

enum net_verdict net_packet_socket_input(struct net_pkt *pkt)
{
	/* Currently we are skipping L2 layer verification and not
	 * removing L2 header from packet.
	 * TODO :
	 * 1) Pass it through L2 layer, so that L2 will verify
	 * that packet is intended to us or not and sets src and dst lladdr.
	 * And L2 should not pull off L2 header when combination of socket
	 * like this AF_PACKET, SOCK_RAW and ETH_P_ALL proto.
	 * 2) Socket combination of AF_INET, SOCK_RAW, IPPROTO_RAW
	 * packet has to go through L2 and L2 verfies it's header and removes
	 * header. Only packet with L3 header will be given to socket.
	 * 3) If user opens raw and non raw socket together, based on raw
	 * socket combination packet has to be feed to raw socket and only
	 * data part to be feed to non raw socket.
	 */

	net_pkt_set_family(pkt, AF_PACKET);

	return net_conn_input(pkt, NULL, ETH_P_ALL, NULL);
}
