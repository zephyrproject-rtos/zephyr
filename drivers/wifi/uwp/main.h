/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WIFI_MAIN_H__
#define __WIFI_MAIN_H__

#include <zephyr.h>
#include <net/wifi_drv.h>


#define WIFI_MODE_NONE (0)
#define WIFI_MODE_STA (1)
#define WIFI_MODE_AP (2)

#define MAX_WIFI_DEV_NUM (2)

#define WIFI_DEV_STA (0)
#define WIFI_DEV_AP (1)

#define ETH_ALEN (6)
#define IPV4_LEN (4)

struct wifi_device {
	bool connected;
	bool opened;
	u8_t mode;
	u8_t mac[ETH_ALEN];
	u8_t ipv4_addr[IPV4_LEN];
	/* Maximum stations on softap */
	u8_t max_sta_num;
	/* Maximum stations in blacklist on softap */
	u8_t max_blacklist_num;
	u8_t max_rtt_num;
	scan_result_evt_t scan_result_cb;
	connect_evt_t connect_cb;
	disconnect_evt_t disconnect_cb;
	new_station_evt_t new_station_cb;
	rtt_result_evt_t rtt_result_cb;
	struct net_if *iface;
	struct device *dev;
};

struct wifi_priv {
	struct wifi_device wifi_dev[MAX_WIFI_DEV_NUM];
	u32_t cp_version;
	bool initialized;
};


#endif /* __WIFI_MAIN_H__ */
