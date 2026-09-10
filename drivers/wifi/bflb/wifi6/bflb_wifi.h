/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_H_
#define BFLB_WIFI_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_stats.h>

struct bflb_wifi_dev {
	struct net_if *iface;
	scan_result_cb_t scan_cb;
	struct k_mutex lock;
	bool initialized;
	bool ap_enabled;
	bool monitor_enabled;
	enum wifi_security_type ap_security;
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_stats_wifi stats;
#endif
};

extern struct bflb_wifi_dev wl80211_dev;

extern int wl80211_set_country_code(const char *country_code);

int bflb_wifi_wait_eapol_tx_done(k_timeout_t timeout);

#endif /* BFLB_WIFI_H_ */
