/*
 * Copyright 2022, Cypress Semiconductor Corporation (an Infineon company)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <whd_types.h>
#include <whd_events_int.h>
#include <whd_buffer_api.h>
#include <zephyr/logging/log.h>
#include <cyw43xxx_drv.h>

LOG_MODULE_REGISTER(infineon_wifi_sta, LOG_LEVEL_DBG);

static struct net_mgmt_event_callback dhcp_cb;
whd_scan_result_t scan_result;
uint32_t num_scan_result;
cy_semaphore_t scan_mutex;
whd_ssid_t ssid;
#define MAX_WIFI_RETRY_COUNT             (3)
#define WIFI_CONN_RETRY_INTERVAL_MSEC    (100u)

#define PRINT_SCAN_TEMPLATE()							  \
	printf("\n--------------------------------------------------------------" \
	       "------------------------------------\n"				  \
	       "  #                  SSID                  RSSI   Channel       " \
	       "MAC Address              Security\n"				  \
	       "----------------------------------------------------------------" \
	       "------------------------------------\n");

#define SECURITY_OPEN                           "OPEN"
#define SECURITY_WEP_PSK                        "WEP-PSK"
#define SECURITY_WEP_SHARED                     "WEP-SHARED"
#define SECURITY_WEP_TKIP_PSK                   "WEP-TKIP-PSK"
#define SECURITY_WPA_TKIP_PSK                   "WPA-TKIP-PSK"
#define SECURITY_WPA_AES_PSK                    "WPA-AES-PSK"
#define SECURITY_WPA_MIXED_PSK                  "WPA-MIXED-PSK"
#define SECURITY_WPA2_AES_PSK                   "WPA2-AES-PSK"
#define SECURITY_WPA2_TKIP_PSK                  "WPA2-TKIP-PSK"
#define SECURITY_WPA2_MIXED_PSK                 "WPA2-MIXED-PSK"
#define SECURITY_WPA2_FBT_PSK                   "WPA2-FBT-PSK"
#define SECURITY_WPA3_SAE                       "WPA3-SAE"
#define SECURITY_WPA2_WPA_AES_PSK               "WPA2-WPA-AES-PSK"
#define SECURITY_WPA2_WPA_MIXED_PSK             "WPA2-WPA-MIXED-PSK"
#define SECURITY_WPA3_WPA2_PSK                  "WPA3-WPA2-PSK"
#define SECURITY_WPA_TKIP_ENT                   "WPA-TKIP-ENT"
#define SECURITY_WPA_AES_ENT                    "WPA-AES-ENT"
#define SECURITY_WPA_MIXED_ENT                  "WPA-MIXED-ENT"
#define SECURITY_WPA2_TKIP_ENT                  "WPA2-TKIP-ENT"
#define SECURITY_WPA2_AES_ENT                   "WPA2-AES-ENT"
#define SECURITY_WPA2_MIXED_ENT                 "WPA2-MIXED-ENT"
#define SECURITY_WPA2_FBT_ENT                   "WPA2-FBT-ENT"
#define SECURITY_IBSS_OPEN                      "IBSS-OPEN"
#define SECURITY_WPS_SECURE                     "WPS-SECURE"
#define SECURITY_UNKNOWN                        "UNKNOWN"

static void handler_cb(struct net_mgmt_event_callback *cb,
		       uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
		return;
	}

	char buf[NET_IPV4_ADDR_LEN];

	LOG_INF("Your address: %s",
		net_addr_ntop(AF_INET,
			      &iface->config.dhcpv4.requested_ip,
			      buf, sizeof(buf)));
	LOG_INF("Lease time: %u seconds",
		iface->config.dhcpv4.lease_time);
	LOG_INF("Subnet: %s",
		net_addr_ntop(AF_INET,
			      &iface->config.ip.ipv4->netmask,
			      buf, sizeof(buf)));
	LOG_INF("Router: %s",
		net_addr_ntop(AF_INET,
			      &iface->config.ip.ipv4->gw,
			      buf, sizeof(buf)));
}

char *convert_securitytype(whd_security_t security)
{
	switch (security) {
	case WHD_SECURITY_OPEN:
		return SECURITY_OPEN;

	case WHD_SECURITY_WEP_PSK:
		return SECURITY_WEP_PSK;

	case WHD_SECURITY_WEP_SHARED:
		return SECURITY_WEP_SHARED;

	case WHD_SECURITY_WPA_TKIP_PSK:
		return SECURITY_WEP_TKIP_PSK;

	case WHD_SECURITY_WPA_AES_PSK:
		return SECURITY_WPA_AES_PSK;

	case WHD_SECURITY_WPA_MIXED_PSK:
		return SECURITY_WPA_MIXED_PSK;

	case WHD_SECURITY_WPA2_AES_PSK:
		return SECURITY_WPA2_AES_PSK;

	case WHD_SECURITY_WPA2_TKIP_PSK:
		return SECURITY_WPA2_TKIP_PSK;

	case WHD_SECURITY_WPA2_MIXED_PSK:
		return SECURITY_WPA2_MIXED_PSK;

	case WHD_SECURITY_WPA2_FBT_PSK:
		return SECURITY_WPA2_FBT_PSK;

	case WHD_SECURITY_WPA3_SAE:
		return SECURITY_WPA3_SAE;

	case WHD_SECURITY_WPA3_WPA2_PSK:
		return SECURITY_WPA3_WPA2_PSK;

	case WHD_SECURITY_IBSS_OPEN:
		return SECURITY_IBSS_OPEN;

	case WHD_SECURITY_WPS_SECURE:
		return SECURITY_WPS_SECURE;

	case WHD_SECURITY_UNKNOWN:
		return SECURITY_UNKNOWN;

	case WHD_SECURITY_WPA2_WPA_AES_PSK:
		return SECURITY_WPA2_WPA_AES_PSK;

	case WHD_SECURITY_WPA2_WPA_MIXED_PSK:
		return SECURITY_WPA2_WPA_MIXED_PSK;

	case WHD_SECURITY_WPA_TKIP_ENT:
		return SECURITY_WPA_TKIP_ENT;

	case WHD_SECURITY_WPA_AES_ENT:
		return SECURITY_WPA_AES_ENT;

	case WHD_SECURITY_WPA_MIXED_ENT:
		return SECURITY_WPA_MIXED_ENT;

	case WHD_SECURITY_WPA2_TKIP_ENT:
		return SECURITY_WPA2_TKIP_ENT;

	case WHD_SECURITY_WPA2_AES_ENT:
		return SECURITY_WPA2_AES_ENT;

	case WHD_SECURITY_WPA2_MIXED_ENT:
		return SECURITY_WPA2_MIXED_ENT;

	case WHD_SECURITY_WPA2_FBT_ENT:
		return SECURITY_WPA2_FBT_ENT;

	default:
		return SECURITY_UNKNOWN;

	}
}

void internal_scan_callback(whd_scan_result_t **result_ptr,
			    void *user_data, whd_scan_status_t status)
{
	whd_scan_result_t whd_scan_result;

	if (status == WHD_SCAN_COMPLETED_SUCCESSFULLY || status == WHD_SCAN_ABORTED) {
		cy_rtos_set_semaphore(&scan_mutex, WHD_FALSE);
		return;
	}
	/* Check if we don't have a scan result to send to the user */
	if ((result_ptr != NULL) || (*result_ptr != NULL)) {
		num_scan_result++;
		memcpy(&whd_scan_result, *result_ptr, sizeof(whd_scan_result_t));
		printf(" %2d   %-32s     %4d     %2d      "
		       "%02X:%02X:%02X:%02X:%02X:%02X         %15s\n",
		       num_scan_result, whd_scan_result.SSID.value,
		       whd_scan_result.signal_strength, whd_scan_result.channel,
		       whd_scan_result.BSSID.octet[0], whd_scan_result.BSSID.octet[1],
		       whd_scan_result.BSSID.octet[2], whd_scan_result.BSSID.octet[3],
		       whd_scan_result.BSSID.octet[4], whd_scan_result.BSSID.octet[5],
		       convert_securitytype(whd_scan_result.security));

	}
	memset(*result_ptr, 0, sizeof(whd_scan_result_t));
}

void wifi_connect(void)
{
	cy_rslt_t ret = CY_RSLT_SUCCESS;
	wl_bss_info_t bss_info;
	whd_security_t security;

	LOG_INF("Connecting to AP [%s] \r\n", CONFIG_CYW43XXX_WIFI_SSID);

	ssid.length = strlen(CONFIG_CYW43XXX_WIFI_SSID);
	memcpy(&ssid.value, CONFIG_CYW43XXX_WIFI_SSID, strlen(CONFIG_CYW43XXX_WIFI_SSID));

	/* Attempt to connect to Wi-Fi until a connection is made or until
	 * MAX_WIFI_RETRY_COUNT attempts have been made.
	 */
	for (uint32_t conn_retries = 0; conn_retries < MAX_WIFI_RETRY_COUNT; conn_retries++) {
		ret = whd_wifi_join(cyw43xx_get_whd_interface(), &ssid, WHD_SECURITY_WPA2_AES_PSK,
				    CONFIG_CYW43XXX_WIFI_PASSWORD,
				    strlen(CONFIG_CYW43XXX_WIFI_PASSWORD));
		if (ret == CY_RSLT_SUCCESS) {
			ret = whd_wifi_get_ap_info(cyw43xx_get_whd_interface(),
						   &bss_info, &security);
			if (ret != CY_RSLT_SUCCESS) {
				LOG_ERR("whd_wifi_get_ap_info failed ret = %d \r\n", ret);
				break;
			}
			LOG_INF("Connected to AP [%s] BSSID : %02X:%02X:%02X:%02X:%02X:%02X , "
				"RSSI %d \r\n", bss_info.SSID,
				bss_info.BSSID.octet[0], bss_info.BSSID.octet[1],
				bss_info.BSSID.octet[2], bss_info.BSSID.octet[3],
				bss_info.BSSID.octet[4], bss_info.BSSID.octet[5],
				bss_info.RSSI);
			break;
		}
		LOG_ERR("Connection to Wi-Fi network failed with error code %d."
			"Retrying in %d ms...\n", ret, WIFI_CONN_RETRY_INTERVAL_MSEC);
		k_msleep(WIFI_CONN_RETRY_INTERVAL_MSEC);
	}
}

void main(void)
{
	struct net_if *iface;
	cy_rslt_t ret = CY_RSLT_SUCCESS;
	whd_mac_t *mac = NULL;

	net_mgmt_init_event_callback(&dhcp_cb, handler_cb,
				     NET_EVENT_IPV4_DHCP_BOUND);

	net_mgmt_add_event_callback(&dhcp_cb);

	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("wifi interface not available");
		return;
	}
	if (cy_rtos_init_semaphore(&scan_mutex, 1, 0) != WHD_SUCCESS) {
		LOG_ERR("connect failed ret = %d \r\n", ret);
		return;
	}
	net_dhcpv4_start(iface);
	PRINT_SCAN_TEMPLATE()
	ret = whd_wifi_scan(cyw43xx_get_whd_interface(), WHD_SCAN_TYPE_ACTIVE, WHD_BSS_TYPE_ANY,
			    &ssid, mac, NULL, NULL, internal_scan_callback, &scan_result, NULL);


	if (cy_rtos_get_semaphore(&scan_mutex, CY_RTOS_NEVER_TIMEOUT, WHD_FALSE) != WHD_SUCCESS) {
		LOG_ERR("Failed to obtain mutex for scan_mutex!\n");
		return;
	}

	/* Wi-Fi Connect */
	wifi_connect();
}
