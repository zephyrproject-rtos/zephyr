/*
 * Copyright (c) 2018, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SUPPORT_H_
#define ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SUPPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <net/wifi_mgmt.h>

#define SSID_LEN_MAX	 (32)
#define BSSID_LEN_MAX	 (6)

/* Define ID for simplelink_wifi_cb to not conflict with WLAN event IDs: */
#define SIMPLELINK_WIFI_CB_IPACQUIRED \
		(SL_WLAN_EVENT_MAX + SL_DEVICE_EVENT_DROPPED_NETAPP_IPACQUIRED)

struct sl_connect_state {
	u32_t gateway_ip;
	u8_t ssid[SSID_LEN_MAX + 1];
	u8_t bssid[BSSID_LEN_MAX];
	u32_t ip_addr;
	u32_t sta_ip;
	u32_t ipv6_addr[4];
	s16_t error;
};

/* Callback from SimpleLink Event Handlers: */
typedef void (*simplelink_wifi_cb_t)(u32_t mgmt_event,
				     struct sl_connect_state *conn);

extern int _simplelink_start_scan(void);
extern void _simplelink_get_scan_result(int index,
					struct wifi_scan_result *scan_result);
extern void _simplelink_get_mac(unsigned char *mac);
extern int _simplelink_init(simplelink_wifi_cb_t wifi_cb);
extern int _simplelink_connect(struct wifi_connect_req_params *params);
extern int _simplelink_disconnect(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SUPPORT_H_ */
