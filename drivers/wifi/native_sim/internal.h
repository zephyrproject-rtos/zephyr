/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_NET_VLAN)
#define ETH_HDR_LEN sizeof(struct net_eth_vlan_hdr)
#else
#define ETH_HDR_LEN sizeof(struct net_eth_hdr)
#endif

struct wifi_context {
	uint8_t recv[NET_ETH_MTU + ETH_HDR_LEN];
	uint8_t send[NET_ETH_MTU + ETH_HDR_LEN];
	uint8_t mac_addr[6];
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	struct net_linkaddr ll_addr;
	struct net_if *iface;
	const char *if_name_host; /* Interface name in host side */
	k_tid_t rx_thread;
	struct z_thread_stack_element *rx_stack;
	size_t rx_stack_size;
	int dev_fd;
	int if_index;
	bool init_done;
	bool status;
	bool promisc_mode;
	bool scan_in_progress;

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats_eth;
#endif
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_stats_wifi stats_wifi;
#endif
};
