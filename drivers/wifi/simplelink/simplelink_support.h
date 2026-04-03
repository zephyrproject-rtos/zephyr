/*
 * Copyright (c) 2018, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SUPPORT_H_
#define ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SUPPORT_H_

#include <zephyr/net/wifi_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSID_LEN_MAX	 (32)
#define BSSID_LEN_MAX	 (6)

/* Define ID for simplelink_wifi_cb to not conflict with WLAN event IDs: */
#define SIMPLELINK_WIFI_CB_IPACQUIRED \
		(SL_WLAN_EVENT_MAX + SL_NETAPP_EVENT_IPV4_ACQUIRED)
#define SIMPLELINK_WIFI_CB_IPV6ACQUIRED \
		(SL_WLAN_EVENT_MAX + SL_NETAPP_EVENT_IPV6_ACQUIRED)

struct sl_connect_state {
	uint32_t gateway_ip;
	uint8_t ssid[SSID_LEN_MAX + 1];
	uint8_t bssid[BSSID_LEN_MAX];
	uint32_t ip_addr;
	uint32_t sta_ip;
	uint32_t ipv6_addr[4];
	int16_t error;
};

/* Callback from SimpleLink Event Handlers: */
typedef void (*simplelink_wifi_cb_t)(uint32_t mgmt_event,
				     struct sl_connect_state *conn);

extern int z_simplelink_start_scan(void);
extern void z_simplelink_get_scan_result(int index,
					struct wifi_scan_result *scan_result);
extern void z_simplelink_get_mac(unsigned char *mac);
extern int z_simplelink_init(simplelink_wifi_cb_t wifi_cb);
extern int z_simplelink_connect(struct wifi_connect_req_params *params);
extern int z_simplelink_disconnect(void);

int simplelink_socket_create(int family, int type, int proto);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SUPPORT_H_ */
