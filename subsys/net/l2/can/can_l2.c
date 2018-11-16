/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <misc/printk.h>
#include <kernel.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/net_context.h>
#include <net/net_l2.h>
#include <zephyr.h>
#include <device.h>

enum net_verdict can_l2_recv(struct net_if *iface,
			     struct net_pkt *pkt)
{
	return NET_CONTINUE;
}

enum net_verdict can_l2_send(struct net_if *iface,
			     struct net_pkt *pkt)
{
	net_if_queue_tx(iface, pkt);

	return NET_OK;
}

u16_t can_l2_reserve(struct net_if *iface, void *unused)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(unused);

	return 0;
}

