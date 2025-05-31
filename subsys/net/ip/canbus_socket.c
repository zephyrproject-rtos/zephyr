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

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/socketcan.h>
#include <zephyr/net/ethernet.h>

#include "connection.h"

static enum net_verdict canbus_l3_recv(struct net_if *iface, uint16_t ptype, struct net_pkt *pkt)
{
	__ASSERT_NO_MSG(net_pkt_family(pkt) == AF_CAN);

	return net_conn_can_input(pkt, CAN_RAW);
}

NET_L3_REGISTER(&NET_L2_GET_NAME(CANBUS_RAW), CAN, ETH_P_CAN, canbus_l3_recv);
NET_L3_REGISTER(&NET_L2_GET_NAME(CANBUS_RAW), CANFD, ETH_P_CANFD, canbus_l3_recv);
