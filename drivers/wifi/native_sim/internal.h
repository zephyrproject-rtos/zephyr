/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_NATIVE_INTERNAL_H_
#define ZEPHYR_INCLUDE_NET_WIFI_NATIVE_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_NET_VLAN)
#define ETH_HDR_LEN sizeof(struct net_eth_vlan_hdr)
#else
#define ETH_HDR_LEN sizeof(struct net_eth_hdr)
#endif

#include "drivers/driver_zephyr.h" /* from hostap */

struct wifi_context;

typedef void (*scan_results_t)(struct wifi_context *ctx,
			       struct wpa_scan_results *results);

struct wifi_context {
	uint8_t recv[NET_ETH_MTU + ETH_HDR_LEN];
	uint8_t send[NET_ETH_MTU + ETH_HDR_LEN];
	uint8_t mac_addr[6];
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	struct net_linkaddr ll_addr;
	struct net_if *iface;
	const char *if_name_host; /* Interface name in host side */
	void *host_context;
	scan_results_t my_scan_cb;
	scan_result_cb_t scan_cb;
	struct k_work_delayable scan_work;
	struct k_work_delayable scan_result_work;
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

	/* Supplicant specific information */
	void *supplicant_drv_ctx;
	struct zep_wpa_supp_dev_callbk_fns supplicant_callbacks;
};

#endif /* ZEPHYR_INCLUDE_NET_WIFI_NATIVE_INTERNAL_H_ */
