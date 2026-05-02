/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAPD_EVENTS_H__
#define __HAPD_EVENTS_H__

#include <zephyr/net/wifi_mgmt.h>

int hostapd_send_wifi_mgmt_ap_status(struct hostapd_iface *iface,
				     enum net_event_wifi_cmd event,
				     enum wifi_ap_status ap_status);

int hostapd_send_wifi_mgmt_ap_sta_event(struct hostapd_iface *ap_ctx,
					enum net_event_wifi_cmd event,
					void *data);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
void hostapd_handle_dpp_event(void *ctx, char *buf, size_t len);
#endif

#endif /* __HAPD_EVENTS_H_ */
