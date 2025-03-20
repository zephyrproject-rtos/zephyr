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
#if defined(CONFIG_NET_DSA_DEPRECATED)
#include <zephyr/net/dsa.h>
#endif

#include "connection.h"
#include "packet_socket.h"

enum net_verdict net_packet_socket_input(struct net_pkt *pkt, uint16_t proto)
{
	sa_family_t orig_family;
	enum net_verdict net_verdict;

#if defined(CONFIG_NET_DSA_DEPRECATED)
	/*
	 * For DSA the master port is not supporting raw packets. Only the
	 * lan1..3 are working with them.
	 */
	if (dsa_is_port_master(net_pkt_iface(pkt))) {
		return NET_CONTINUE;
	}
#endif

	orig_family = net_pkt_family(pkt);

	net_pkt_set_family(pkt, AF_PACKET);

	net_verdict = net_conn_packet_input(pkt, proto);

	net_pkt_set_family(pkt, orig_family);

	return net_verdict;
}
