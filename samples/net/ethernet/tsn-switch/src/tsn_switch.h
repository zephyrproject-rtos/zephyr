/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLE_TSN_SWITCH_H_
#define ZEPHYR_SAMPLE_TSN_SWITCH_H_

#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/net_config.h>
#include <zephyr/drivers/ptp_clock.h>

struct ud {
	struct net_if *bridge;
	struct net_if *iface[CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT];
};

extern struct ud g_user_data;

#endif
