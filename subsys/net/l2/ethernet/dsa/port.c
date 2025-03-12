/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/dsa.h>
#include <zephyr/net/ethernet.h>

#ifdef CONFIG_NET_L2_ETHERNET
bool dsa_port_is_master(struct net_if *iface)
{
	/* First check if iface points to ETH interface */
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		/* Check its capabilities */
		if (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_MASTER_PORT) {
			return true;
		}
	}

	return false;
}
#else
bool dsa_port_is_master(struct net_if *iface)
{
	return false;
}
#endif
