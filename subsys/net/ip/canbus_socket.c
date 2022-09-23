/** @file
 * @brief CANBUS Sockets related functions
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sockets_can, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/socketcan.h>

#include "connection.h"

enum net_verdict net_canbus_socket_input(struct net_pkt *pkt)
{
	__ASSERT_NO_MSG(net_pkt_family(pkt) == AF_CAN);

	if (net_if_l2(net_pkt_iface(pkt)) == &NET_L2_GET_NAME(CANBUS_RAW)) {
		return net_conn_input(pkt, NULL, CAN_RAW, NULL);
	}

	return NET_CONTINUE;
}
