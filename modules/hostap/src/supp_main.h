/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SUPP_MAIN_H_
#define __SUPP_MAIN_H_

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
/* At least one of the EAP methods need to be enabled in enterprise mode */
#if !defined(CONFIG_EAP_TLS) && !defined(CONFIG_EAP_TTLS) && \
	!defined(CONFIG_EAP_PEAP) && !defined(CONFIG_EAP_FAST) && \
	!defined(CONFIG_EAP_SIM) && !defined(CONFIG_EAP_AKA) && \
	!defined(CONFIG_EAP_MD5) && !defined(CONFIG_EAP_MSCHAPV2) && \
	!defined(CONFIG_EAP_PSK) && !defined(CONFIG_EAP_PAX) && \
	!defined(CONFIG_EAP_SAKE) && !defined(CONFIG_EAP_GPSK) && \
	!defined(CONFIG_EAP_PWD) && !defined(CONFIG_EAP_EKE) && \
	!defined(CONFIG_EAP_IKEV2) && !defined(CONFIG_EAP_GTC) && \
	!defined(CONFIG_EAP_LEAP)
#error "At least one of the following EAP methods need to be defined    \
	CONFIG_EAP_TLS    \
	CONFIG_EAP_TTLS   \
	CONFIG_EAP_PEAP   \
	CONFIG_EAP_MD5        \
	CONFIG_EAP_MSCHAPV2    \
	CONFIG_EAP_LEAP    \
	CONFIG_EAP_PSK   \
	CONFIG_EAP_PAX   \
	CONFIG_EAP_SAKE   \
	CONFIG_EAP_GPSK   \
	CONFIG_EAP_PWD   \
	CONFIG_EAP_EKE   \
	CONFIG_EAP_IKEV2   \
	CONFIG_EAP_SIM   \
	CONFIG_EAP_AKA   \
	CONFIG_EAP_GTC   \
	CONFIG_EAP_ALL "
#endif /* EAP METHODS */
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */

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
struct hapd_interfaces *zephyr_get_default_hapd_context(void);
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
