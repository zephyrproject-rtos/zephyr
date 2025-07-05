/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_canbus, LOG_LEVEL_NONE);

#include <zephyr/drivers/can.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/canbus.h>
#include <zephyr/net/ethernet.h>

static inline enum net_verdict canbus_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct can_frame frame;

	memset(net_pkt_lladdr_src(pkt)->addr, 0, sizeof(net_pkt_lladdr_src(pkt)->addr));
	net_pkt_lladdr_src(pkt)->len = 0U;
	net_pkt_lladdr_src(pkt)->type = NET_LINK_CANBUS_RAW;
	memset(net_pkt_lladdr_dst(pkt)->addr, 0, sizeof(net_pkt_lladdr_dst(pkt)->addr));
	net_pkt_lladdr_dst(pkt)->len = 0U;
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_CANBUS_RAW;

	if (net_pkt_get_len(pkt) < sizeof(struct can_frame)) {
		return NET_DROP;
	}

	memcpy(&frame, net_pkt_data(pkt), sizeof(struct can_frame));
	if (frame.flags & CAN_FRAME_FDF) {
		net_pkt_set_ll_proto_type(pkt, ETH_P_CANFD);
	} else {
		net_pkt_set_ll_proto_type(pkt, ETH_P_CAN);
	}

	net_pkt_set_family(pkt, AF_CAN);

	return NET_CONTINUE;
}

static inline int canbus_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct canbus_api *api = net_if_get_device(iface)->api;
	int ret;

	if (!api) {
		return -ENOENT;
	}

	ret = net_l2_send(api->send, net_if_get_device(iface), iface, pkt);
	if (!ret) {
		ret = net_pkt_get_len(pkt);
		net_pkt_unref(pkt);
	}

	return ret;
}

NET_L2_INIT(CANBUS_RAW_L2, canbus_recv, canbus_send, NULL, NULL);
