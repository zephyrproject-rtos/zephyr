/*
 * @file
 * @brief WiFi manager header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_H_
#define _WIFIMGR_H_

#include <net/wifi_drv.h>
#include <net/wifi_api.h>

#include "os_adapter.h"
#include "config.h"
#include "cmd_prcs.h"
#include "ctrl_iface.h"
#include "drv_iface.h"
#include "evt_lsnr.h"
#include "timer.h"
#include "sm.h"
#include "psk.h"
#include "led.h"

#ifdef CONFIG_WIFI_STA_DRV_NAME
#define WIFIMGR_DEV_NAME_STA	CONFIG_WIFI_STA_DRV_NAME
#else
#define WIFIMGR_DEV_NAME_STA	"WIFI_STA"
#endif

#ifdef CONFIG_WIFI_AP_DRV_NAME
#define WIFIMGR_DEV_NAME_AP	CONFIG_WIFI_AP_DRV_NAME
#else
#define WIFIMGR_DEV_NAME_AP	"WIFI_AP"
#endif

#define WIFIMGR_MAX_RTT_NR	10
#define WIFIMGR_MAX_STA_NR	16

#define C2S(x) case x: return #x;

struct wifimgr_mac_node {
	wifimgr_snode_t node;
	char mac[WIFI_MAC_ADDR_LEN];
};

struct wifimgr_mac_list {
	unsigned char nr;
	wifimgr_slist_t list;
};

struct wifimgr_sta_event {
	union {
		char evt_status;
		struct wifi_drv_scan_result_evt scan_res;
		struct wifi_drv_rtt_response_evt rtt_resp;
		struct wifi_drv_connect_evt conn;
	} u;
};

struct wifimgr_ap_event {
	union {
		struct wifi_drv_new_station_evt new_sta;
	} u;
};

struct wifi_manager {
	union wifi_drv_capa sta_capa;
	struct wifi_config sta_conf;
	struct wifi_status sta_sts;
	struct wifimgr_state_machine sta_sm;
	struct wifi_scan_params sta_scan_params;
	struct wifi_scan_result sta_scan_res;
	struct wifi_rtt_request sta_rtt_req;
	struct wifi_rtt_response sta_rtt_resp;
	struct wifimgr_ctrl_iface sta_ctrl;
	struct wifimgr_delayed_work sta_autowork;

	union wifi_drv_capa ap_capa;
	struct wifi_config ap_conf;
	struct wifi_status ap_sts;
	struct wifimgr_state_machine ap_sm;
	struct wifimgr_mac_list assoc_list;
	struct wifimgr_mac_list mac_acl;
	struct wifimgr_set_mac_acl set_acl;
	struct wifimgr_ctrl_iface ap_ctrl;
	struct wifimgr_delayed_work ap_autowork;

	struct cmd_processor prcs;
	struct evt_listener lsnr;

	void *sta_iface;
	void *ap_iface;

	struct wifimgr_sta_event sta_evt;
	struct wifimgr_ap_event ap_evt;
};

#ifdef CONFIG_WIFIMGR_STA
int wifimgr_sta_init(void *handle);
void wifimgr_sta_exit(void *handle);
#else
static inline int wifimgr_sta_init(void *handle)
{
	return 0;
}
#define wifimgr_sta_exit(...)
#endif

#ifdef CONFIG_WIFIMGR_AP
int wifimgr_ap_init(void *handle);
void wifimgr_ap_exit(void *handle);
#else
static inline int wifimgr_ap_init(void *handle)
{
	return 0;
}
#define wifimgr_ap_exit(...)
#endif

#ifdef CONFIG_WIFIMGR_AUTORUN
int wifi_autorun_init(void);
#else
static inline int wifi_autorun_init(void)
{
	return 0;
}
#endif

#endif
