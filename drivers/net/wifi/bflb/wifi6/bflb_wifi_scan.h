/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_SCAN_H_
#define BFLB_WIFI_SCAN_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

struct bflb_wifi_dev;
struct wl80211_scan_result_item;

void bflb_privacy_cache_install_hook(void);
bool bflb_privacy_cache_lookup(const uint8_t *bssid);
const uint8_t *bflb_rsn_ie_cache_lookup(const uint8_t *bssid, uint8_t *out_len);
const uint8_t *bflb_rsnxe_cache_lookup(const uint8_t *bssid, uint8_t *out_len);

uint8_t *bflb_append_security_ie(uint8_t *pos, struct wl80211_scan_result_item *n);

void bflb_wifi_deliver_scan_results(struct bflb_wifi_dev *wdev);
int bflb_wifi_scan(const struct device *dev, struct net_if *iface, struct wifi_scan_params *params,
		   scan_result_cb_t cb);

#endif /* BFLB_WIFI_SCAN_H_ */
