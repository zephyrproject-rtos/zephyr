/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SUPP_MAIN_H_
#define __SUPP_MAIN_H_

#if !defined(CONFIG_NET_DHCPV4)
static inline void net_dhcpv4_start(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
static inline void net_dhcpv4_stop(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#else
#include <zephyr/net/dhcpv4.h>
#endif

struct wpa_global *zephyr_get_default_supplicant_context(void);
struct wpa_supplicant *zephyr_get_handle_by_ifname(const char *ifname);
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
struct hostapd_iface *zephyr_get_hapd_handle_by_ifname(const char *ifname);
#endif
struct wpa_supplicant_event_msg {
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	int hostapd;
#endif
	bool global;
	void *ctx;
	unsigned int event;
	void *data;
};
int zephyr_wifi_send_event(const struct wpa_supplicant_event_msg *msg);
#endif /* __SUPP_MAIN_H_ */
