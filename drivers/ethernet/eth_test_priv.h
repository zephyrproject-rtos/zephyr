/*
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_TEST_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_TEST_PRIV_H_

#include <zephyr/net/ethernet.h>

struct vnd_ethernet_config {
	struct net_eth_mac_config mcfg;
};

struct vnd_ethernet_data {
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	int mac_addr_load_result;
};

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_TEST_PRIV_H_ */
