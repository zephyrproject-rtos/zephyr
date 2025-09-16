/*
 * Copyright (c) 2024 Muhammad Haziq
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>

LOG_MODULE_REGISTER(MAIN);

#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define NET_EVENT_WIFI_MASK                                                                        \
	(NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT |                        \
	 NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT |                      \
	 NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

static struct net_if *ap_iface;
static struct net_if *sta_iface;

static struct wifi_connect_req_params ap_config;
static struct wifi_connect_req_params sta_config;

static struct net_mgmt_event_callback cb;

/* Check necessary definitions */

BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_SSID) > 1,
	     "CONFIG_WIFI_SAMPLE_AP_SSID is empty. Please set it in conf file.");

BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_SSID) > 1,
	     "CONFIG_WIFI_SAMPLE_SSID is empty. Please set it in conf file.");

#if WIFI_SAMPLE_DHCPV4_START
BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS) > 1,
	     "CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS is empty. Please set it in conf file.");

BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_NETMASK) > 1,
	     "CONFIG_WIFI_SAMPLE_AP_NETMASK is empty. Please set it in conf file.");

#endif

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		LOG_INF("Connected to %s", CONFIG_WIFI_SAMPLE_SSID);
		break;
	}
	case NET_EVENT_WIFI_DISCONNECT_RESULT: {
		LOG_INF("Disconnected from %s", CONFIG_WIFI_SAMPLE_SSID);
		break;
	}
	case NET_EVENT_WIFI_AP_ENABLE_RESULT: {
		LOG_INF("AP Mode is enabled. Waiting for station to connect");
		break;
	}
	case NET_EVENT_WIFI_AP_DISABLE_RESULT: {
		LOG_INF("AP Mode is disabled.");
		break;
	}
	case NET_EVENT_WIFI_AP_STA_CONNECTED: {
		struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

		LOG_INF("station: " MACSTR " joined ", sta_info->mac[0], sta_info->mac[1],
			sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
		break;
	}
	case NET_EVENT_WIFI_AP_STA_DISCONNECTED: {
		struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

		LOG_INF("station: " MACSTR " leave ", sta_info->mac[0], sta_info->mac[1],
			sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
		break;
	}
	default:
		break;
	}
}

#if CONFIG_WIFI_SAMPLE_DHCPV4_START
static void enable_dhcpv4_server(void)
{
	static struct in_addr addr;
	static struct in_addr netmaskAddr;

	if (net_addr_pton(AF_INET, CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS, &addr)) {
		LOG_ERR("Invalid address: %s", CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS);
		return;
	}

	if (net_addr_pton(AF_INET, CONFIG_WIFI_SAMPLE_AP_NETMASK, &netmaskAddr)) {
		LOG_ERR("Invalid netmask: %s", CONFIG_WIFI_SAMPLE_AP_NETMASK);
		return;
	}

	net_if_ipv4_set_gw(ap_iface, &addr);

	if (net_if_ipv4_addr_add(ap_iface, &addr, NET_ADDR_MANUAL, 0) == NULL) {
		LOG_ERR("unable to set IP address for AP interface");
	}

	if (!net_if_ipv4_set_netmask_by_addr(ap_iface, &addr, &netmaskAddr)) {
		LOG_ERR("Unable to set netmask for AP interface: %s",
			 CONFIG_WIFI_SAMPLE_AP_NETMASK);
	}

	addr.s4_addr[3] += 10; /* Starting IPv4 address for DHCPv4 address pool. */

	if (net_dhcpv4_server_start(ap_iface, &addr) != 0) {
		LOG_ERR("DHCP server is not started for desired IP");
		return;
	}

	LOG_INF("DHCPv4 server started...\n");
}
#endif

static int enable_ap_mode(void)
{
	if (!ap_iface) {
		LOG_INF("AP: is not initialized");
		return -EIO;
	}

	LOG_INF("Turning on AP Mode");
	ap_config.ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_AP_SSID;
	ap_config.ssid_length = sizeof(CONFIG_WIFI_SAMPLE_AP_SSID) - 1;
	ap_config.psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_AP_PSK;
	ap_config.psk_length = sizeof(CONFIG_WIFI_SAMPLE_AP_PSK) - 1;
	ap_config.channel = WIFI_CHANNEL_ANY;
	ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;

	if (sizeof(CONFIG_WIFI_SAMPLE_AP_PSK) == 1) {
		ap_config.security = WIFI_SECURITY_TYPE_NONE;
	} else {

		ap_config.security = WIFI_SECURITY_TYPE_PSK;
	}

#if CONFIG_WIFI_SAMPLE_DHCPV4_START
	enable_dhcpv4_server();
#endif

	int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, ap_iface, &ap_config,
			   sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("NET_REQUEST_WIFI_AP_ENABLE failed, err: %d", ret);
	}

	return ret;
}

static int connect_to_wifi(void)
{
	if (!sta_iface) {
		LOG_INF("STA: interface no initialized");
		return -EIO;
	}

	sta_config.ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_SSID;
	sta_config.ssid_length = sizeof(CONFIG_WIFI_SAMPLE_SSID) - 1;
	sta_config.psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_PSK;
	sta_config.psk_length = sizeof(CONFIG_WIFI_SAMPLE_PSK) - 1;
	sta_config.security = WIFI_SECURITY_TYPE_PSK;
	sta_config.channel = WIFI_CHANNEL_ANY;
	sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;

	LOG_INF("Connecting to SSID: %s\n", sta_config.ssid);

	int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
			   sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("Unable to Connect to (%s)", CONFIG_WIFI_SAMPLE_SSID);
	}

	return ret;
}

int main(void)
{
	k_sleep(K_SECONDS(5));

	net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&cb);

	/* Get AP interface in AP-STA mode. */
	ap_iface = net_if_get_wifi_sap();

	/* Get STA interface in AP-STA mode. */
	sta_iface = net_if_get_wifi_sta();

	enable_ap_mode();
	connect_to_wifi();

	return 0;
}
