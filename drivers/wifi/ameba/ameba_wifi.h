/*
 * Copyright (c) 2026 Realtek Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_AMEBA_AMEBA_WIFI_H_
#define ZEPHYR_DRIVERS_WIFI_AMEBA_AMEBA_WIFI_H_

#include "os_wrapper_semaphore.h"
#include "rtw_wifi_defs.h"
#include "wifi_intf_drv_to_app_basic.h"
#include "os_wrapper_memory.h"

#define DT_DRV_COMPAT         realtek_ameba_wifi
#define DHCPV4_MASK           (NET_EVENT_IPV4_DHCP_BOUND | NET_EVENT_IPV4_DHCP_STOP)
#define MAX_IP_ADDR_LEN       16

struct rtk_wifi_status {
	char ssid[WIFI_SSID_MAX_LEN + 1];
	char pass[WIFI_PSK_MAX_LEN + 1];
	bool connected;
	uint8_t channel;
	int rssi;
} __packed;

struct ameba_wifi_runtime {
	uint8_t mac_addr[6];
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_stats_wifi stats;
#endif
	struct rtk_wifi_status status;
	scan_result_cb_t scan_cb;
	uint8_t if_idx;
	uint8_t state;
	uint8_t dhcp_init;
};

enum ameba_system_event_id {
	SYSTEM_EVENT_WIFI_READY = 0,
	SYSTEM_EVENT_SCAN_DONE,
	SYSTEM_EVENT_STA_START,
	SYSTEM_EVENT_STA_STOP,
	SYSTEM_EVENT_STA_CONNECTED,
	SYSTEM_EVENT_STA_DISCONNECTED,

	RTK_WIFI_EVENT_STA_START,
	RTK_WIFI_EVENT_STA_STOP,
	RTK_WIFI_EVENT_STA_CONNECTED,
	RTK_WIFI_EVENT_STA_DISCONNECTED,
	RTK_WIFI_EVENT_SCAN_DONE,
	RTK_WIFI_EVENT_AP_STOP,
	RTK_WIFI_EVENT_AP_STACONNECTED,
	RTK_WIFI_EVENT_AP_STADISCONNECTED,
};

struct ameba_wifi_event_sta_connected {
	uint8_t bssid[6]; /*!< BSSID of the connected AP */
	uint8_t ssid[32]; /*!< SSID of the connected AP */
	uint8_t ssid_len; /*!< Length of the SSID */
	uint8_t channel;  /*!< Channel of the connected AP */
} __packed;

union ameba_system_event_info {
	struct ameba_wifi_event_sta_connected sta_connected; /*!< station connected to AP */
} __packed;

struct ameba_system_event {
	enum ameba_system_event_id event_id;
	union ameba_system_event_info event_info;
} __packed;

enum rtk_state_flag {
	RTK_STA_STOPPED = 0,
	RTK_AP_STOPPED = 0,
	RTK_STA_STARTED,
	RTK_STA_CONNECTING,
	RTK_STA_CONNECTED,
	RTK_STAAP_STARTED,
	RTK_AP_CONNECTED,
	RTK_AP_DISCONNECTED,
};

void wifi_init(void);
int wifi_get_scan_records(unsigned int *AP_num, char *scan_buf);
int wifi_get_mac_address(int idx, struct _rtw_mac_t *mac, uint8_t efuse);
int rltk_wlan_send(int idx, void *pkt_addr, uint32_t len);
int whc_host_send_zephyr(int idx, void *pkt_addr, uint32_t len);
void wlan_int_enable(void);
int wifi_get_setting_zephyr(uint8_t idx, char *ssid, uint8_t *ssid_len, char *bssid, int *channel,
			    uint32_t *security);
int wifi_connect_zephyr(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk, uint8_t psk_len,
			uint8_t channel);
int wifi_start_ap_zephyr(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk, uint8_t psk_len,
			 uint8_t channel, uint8_t wpa3_en);


#endif /* ZEPHYR_DRIVERS_WIFI_AMEBA_AMEBA_WIFI_H_ */
