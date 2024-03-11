/**
 * @file
 * @brief Ethernet Driver raw mode
 *
 * This file contains a collection of functions called from the ethernet drivers
 * to the missing upper layer.
 */

/*
 * Copyright 2024 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/ethernet.h>

__weak void ethernet_init(struct net_if *iface)
{
	ARG_UNUSED(iface);
}

__weak void net_eth_carrier_on(struct net_if *iface)
{
	ARG_UNUSED(iface);
}

__weak void net_eth_carrier_off(struct net_if *iface)
{
	ARG_UNUSED(iface);
}

__weak int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);

	return -ENOTSUP;
}

__weak void net_if_carrier_on(struct net_if *iface)
{
	ARG_UNUSED(iface);
}

__weak void net_if_carrier_off(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
