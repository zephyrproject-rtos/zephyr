/*
 * Copyright (c) 2024 Realtek Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ameba_wifi, CONFIG_WIFI_LOG_LEVEL);

#if defined(CONFIG_BT_COEXIST)
#include "rtw_coex_ipc.h"
#endif
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/device.h>
#include <soc.h>
#include "diag.h"

#if !defined(SOC_SERIES_AMEBAD)
#include <zephyr/settings/settings.h>
#endif

#include "ameba_wifi.h"
#include <zephyr/net/wifi_nm.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>
/* use global iface pointer to support any ethernet driver */
/* necessary for wifi callback functions */
static struct net_if *ameba_wifi_iface[2];
static struct ameba_wifi_runtime ameba_data[2];

extern void (*p_wifi_join_info_free)(u8 iface_type);

K_MSGQ_DEFINE(ameba_wifi_msgq, sizeof(struct ameba_system_event), 10, 4);
#define WIFI_EVENT_STACK_SIZE CONFIG_AMEBA_WIFI_EVENT_STACK_SIZE
K_THREAD_STACK_DEFINE(ameba_wifi_event_stack, WIFI_EVENT_STACK_SIZE);

static struct k_thread ameba_wifi_event_thread;
/* for add dhcp callback in mgmt_thread in net_mgmt */
static struct net_mgmt_event_callback ameba_dhcp_cb = {0};

static int dev_init_done;
static unsigned char if_init_idx;

#ifndef CONFIG_SINGLE_CORE_WIFI
extern int (*rx_callback_ptr)(uint8_t idx, void *buffer, uint16_t len);
extern void (*tx_read_pkt_ptr)(void *pkt_addr, void *data, size_t length);

void skb_read_pkt(void *pkt_addr, void *data, size_t length)
{
	net_pkt_read((struct net_pkt *)pkt_addr, data, length);
}

static void ameba_wifi_internal_reg_rxcb(uint32_t idx, void *cb)
{
	rx_callback_ptr = cb;
	tx_read_pkt_ptr = skb_read_pkt;
}
#endif

static void print_scan_result(struct rtw_scan_result *record)
{
	DiagPrintf("" MAC_FMT ",", MAC_ARG(record->BSSID.octet));
	DiagPrintf(" %d\t ", record->signal_strength);
	DiagPrintf(" %d\t  ", record->channel);
	DiagPrintf("%s\t\t ",
		   (record->security == RTW_SECURITY_OPEN)                 ? "Open"
		   : (record->security == RTW_SECURITY_WEP_PSK)            ? "WEP"
		   : (record->security == RTW_SECURITY_WPA_TKIP_PSK)       ? "WPA TKIP"
		   : (record->security == RTW_SECURITY_WPA_AES_PSK)        ? "WPA AES"
		   : (record->security == RTW_SECURITY_WPA_MIXED_PSK)      ? "WPA Mixed"
		   : (record->security == RTW_SECURITY_WPA2_AES_PSK)       ? "WPA2 AES"
		   : (record->security == RTW_SECURITY_WPA2_TKIP_PSK)      ? "WPA2 TKIP"
		   : (record->security == RTW_SECURITY_WPA2_MIXED_PSK)     ? "WPA2 Mixed"
		   : (record->security == RTW_SECURITY_WPA_WPA2_TKIP_PSK)  ? "WPA/WPA2 TKIP"
		   : (record->security == RTW_SECURITY_WPA_WPA2_AES_PSK)   ? "WPA/WPA2 AES"
		   : (record->security == RTW_SECURITY_WPA_WPA2_MIXED_PSK) ? "WPA/WPA2 Mixed"
		   : (record->security == (RTW_SECURITY_WPA_TKIP_PSK | ENTERPRISE_ENABLED))
			   ? "WPA TKIP Enterprise"
		   : (record->security == (RTW_SECURITY_WPA_AES_PSK | ENTERPRISE_ENABLED))
			   ? "WPA AES Enterprise"
		   : (record->security == (RTW_SECURITY_WPA_MIXED_PSK | ENTERPRISE_ENABLED))
			   ? "WPA Mixed Enterprise"
		   : (record->security == (RTW_SECURITY_WPA2_TKIP_PSK | ENTERPRISE_ENABLED))
			   ? "WPA2 TKIP Enterprise"
		   : (record->security == (RTW_SECURITY_WPA2_AES_PSK | ENTERPRISE_ENABLED))
			   ? "WPA2 AES Enterprise"
		   : (record->security == (RTW_SECURITY_WPA2_MIXED_PSK | ENTERPRISE_ENABLED))
			   ? "WPA2 Mixed Enterprise"
		   : (record->security == (RTW_SECURITY_WPA_WPA2_TKIP_PSK | ENTERPRISE_ENABLED))
			   ? "WPA/WPA2 TKIP Enterprise"
		   : (record->security == (RTW_SECURITY_WPA_WPA2_AES_PSK | ENTERPRISE_ENABLED))
			   ? "WPA/WPA2 AES Enterprise"
		   : (record->security == (RTW_SECURITY_WPA_WPA2_MIXED_PSK | ENTERPRISE_ENABLED))
			   ? "WPA/WPA2 Mixed Enterprise"
		   : (record->security == RTW_SECURITY_WPA3_AES_PSK)    ? "WPA3-SAE AES"
		   : (record->security == RTW_SECURITY_WPA2_WPA3_MIXED) ? "WPA2/WPA3-SAE AES"
		   : (record->security == (WPA3_SECURITY | ENTERPRISE_ENABLED)) ? "WPA3 Enterprise"
		   : (record->security == RTW_SECURITY_WPA3_OWE)                ? "WPA3-OWE"
										: "Unknown");

	DiagPrintf(" %s ", record->SSID.val);
	if (record->bss_type == RTW_BSS_TYPE_WTN_HELPER) {
		DiagPrintf(" Helper\t ");
	}
	DiagPrintf("\r\n");
}

/* called in mgmt_thread in net_mgmt */
static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_DHCP_BOUND) {
		wifi_mgmt_raise_connect_result_event(iface, 0);
	}
}

/* for sema give to rtk_wifi_event_task */
int ameba_event_send_internal(int32_t event_id, void *event_data, size_t event_data_size,
			      uint32_t ticks_to_wait)
{
	struct ameba_system_event evt;

	evt.event_id = event_id;

	if (event_data_size > sizeof(evt.event_info)) {
		LOG_ERR("MSG %d not find %d > %d", event_id, event_data_size,
			sizeof(evt.event_info));
		return -EIO;
	}

	memcpy(&evt.event_info, event_data, event_data_size);
	k_msgq_put(&ameba_wifi_msgq, &evt, K_FOREVER);
	return 0;
}

static int ameba_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	const int pkt_len = net_pkt_get_len(pkt);
	int idx;

	struct net_if *iface = net_pkt_iface(pkt);

	if (ameba_wifi_iface[STA_WLAN_INDEX] == iface) {
		idx = STA_WLAN_INDEX;
	} else {
		idx = SOFTAP_WLAN_INDEX;
	}

#if defined(CONFIG_WHC_HOST)
	whc_host_send_zephyr(idx, pkt, pkt_len);
#else
	rltk_wlan_send(idx, pkt, pkt_len);
#endif

	return 0;
}

/* should called in driver after rx */
static int eth_rtk_rx(uint8_t idx, void *buffer, uint16_t len)
{
	struct net_pkt *pkt;

	if (ameba_wifi_iface[idx] == NULL) {
		LOG_ERR("network interface unavailable");
		return -EIO;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ameba_wifi_iface[idx], len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to get net buffer");
		return -EIO;
	}

	if (net_pkt_write(pkt, buffer, len) < 0) {
		LOG_ERR("Failed to write pkt");
		goto pkt_unref;
	}
	if (net_recv_data(ameba_wifi_iface[idx], pkt) < 0) {
		LOG_ERR("Failed to push received data");
		goto pkt_unref;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	ameba_data[idx].stats.bytes.received += len;
	ameba_data[idx].stats.pkts.rx++;
#endif

	return 0;

pkt_unref:
	net_pkt_unref(pkt);

#if defined(CONFIG_NET_STATISTICS_WIFI)
	ameba_data[idx].stats.errors.rx++;
#endif

	return -EIO;
}

u8 *ameba_wifi_get_ip(u8 idx)
{
	struct net_if_addr *ifaddr;

	for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		ifaddr = (struct net_if_addr *)&ameba_wifi_iface[idx]->config.ip.ipv4->unicast[i];
		if (ifaddr->is_used) {
			return (u8 *)&ifaddr->address.in_addr;
		}
	}

	return 0;
}

u8 *ameba_wifi_get_gw(u8 idx)
{
	return (u8 *)&ameba_wifi_iface[idx]->config.ip.ipv4->gw;
}

static int ameba_scan_done_cb(unsigned int scanned_AP_num, void *user_data)
{
	struct wifi_scan_result res;
	struct rtw_scan_result *scanned_AP_info;
	char *scan_buf = NULL;

	/* scanned no AP*/
	if (scanned_AP_num == 0) {
		LOG_DBG("No Wi-Fi AP found");
		goto out;
	}

	scan_buf = rtos_mem_zmalloc(scanned_AP_num * sizeof(struct rtw_scan_result));
	if (scan_buf == NULL) {
		LOG_DBG("Failed to malloc buffer to print scan results");
		goto out;
	}

	if (wifi_get_scan_records(&scanned_AP_num, scan_buf) < 0) {
		LOG_DBG("Unable to retrieve AP records");
		goto out;
	}

	if (ameba_data[STA_WLAN_INDEX].scan_cb) {
		for (int i = 0; i < scanned_AP_num; i++) {
			scanned_AP_info =
				(struct rtw_scan_result *)(scan_buf +
							   i * (sizeof(struct rtw_scan_result)));
			int ssid_len = scanned_AP_info->SSID.len;

			scanned_AP_info->SSID.val[ssid_len] = 0;
			memset(&res, 0, sizeof(struct wifi_scan_result));
			res.ssid_length = scanned_AP_info->SSID.len;
			strncpy(res.ssid, scanned_AP_info->SSID.val, ssid_len);
			res.rssi = scanned_AP_info->signal_strength;
			res.channel = scanned_AP_info->channel;
			res.security = WIFI_SECURITY_TYPE_NONE;
			if (scanned_AP_info->security > RTW_SECURITY_OPEN) {
				res.security = WIFI_SECURITY_TYPE_PSK;
			}

			print_scan_result(scanned_AP_info);
			/* ensure notifications get delivered */
			k_yield();
		}
	}

out:
	if (scan_buf) {
		k_free(scan_buf);
	}
	/* report end of scan event, callback in mgmt.c to inform upper */
	ameba_data[STA_WLAN_INDEX].scan_cb(ameba_wifi_iface[STA_WLAN_INDEX], 0, NULL);
	ameba_data[STA_WLAN_INDEX].scan_cb = NULL;

	return 0;
}

static void ameba_wifi_handle_connect_event(void)
{
	net_eth_carrier_on(ameba_wifi_iface[STA_WLAN_INDEX]);

	if (IS_ENABLED(CONFIG_RTK_WIFI_STA_AUTO_DHCPV4)) {
		if (ameba_data[STA_WLAN_INDEX].dhcp_init == 0) {
			net_dhcpv4_start(ameba_wifi_iface[STA_WLAN_INDEX]);
			ameba_data[STA_WLAN_INDEX].dhcp_init = 1;
		} else {
			net_dhcpv4_restart(ameba_wifi_iface[STA_WLAN_INDEX]);
		}

	} else {
		wifi_mgmt_raise_connect_result_event(ameba_wifi_iface[STA_WLAN_INDEX], 0);
	}

	ameba_data[STA_WLAN_INDEX].state = RTK_STA_CONNECTED;
}

static void ameba_wifi_handle_disconnect_event(void)
{
	if (ameba_data[STA_WLAN_INDEX].state == RTK_STA_CONNECTED) {
		if (IS_ENABLED(CONFIG_RTK_WIFI_STA_AUTO_DHCPV4)) {
			net_dhcpv4_stop(ameba_wifi_iface[STA_WLAN_INDEX]);
			ameba_data[STA_WLAN_INDEX].dhcp_init = 0;
		}
		wifi_mgmt_raise_disconnect_result_event(ameba_wifi_iface[STA_WLAN_INDEX], 0);
	} else {
		wifi_mgmt_raise_disconnect_result_event(ameba_wifi_iface[STA_WLAN_INDEX], -1);
	}
}

static void ameba_wifi_event_task(void)
{
	struct ameba_system_event evt;
	uint8_t s_con_cnt = 0;

	while (1) {
		k_msgq_get(&ameba_wifi_msgq, &evt, K_FOREVER);

		switch (evt.event_id) {
		case RTK_WIFI_EVENT_STA_START:
			ameba_data[STA_WLAN_INDEX].state = RTK_STA_STARTED;
			net_eth_carrier_on(ameba_wifi_iface[STA_WLAN_INDEX]);
			break;
		case RTK_WIFI_EVENT_STA_STOP:
			ameba_data[STA_WLAN_INDEX].state = RTK_STA_STOPPED;
			net_eth_carrier_off(ameba_wifi_iface[STA_WLAN_INDEX]);
			break;
		case RTK_WIFI_EVENT_STA_CONNECTED:
			ameba_wifi_handle_connect_event();
			break;
		case RTK_WIFI_EVENT_STA_DISCONNECTED:
			ameba_wifi_handle_disconnect_event();
			break;
		case RTK_WIFI_EVENT_SCAN_DONE:
			break;
		case RTK_WIFI_EVENT_AP_STOP:
			ameba_data[SOFTAP_WLAN_INDEX].state = RTK_AP_STOPPED;
			net_eth_carrier_off(ameba_wifi_iface[SOFTAP_WLAN_INDEX]);
			break;
		case RTK_WIFI_EVENT_AP_STACONNECTED:
			ameba_data[SOFTAP_WLAN_INDEX].state = RTK_AP_CONNECTED;
			s_con_cnt++;
			break;
		case RTK_WIFI_EVENT_AP_STADISCONNECTED:
			ameba_data[SOFTAP_WLAN_INDEX].state = RTK_AP_DISCONNECTED;
			break;
		default:
			break;
		}
	}
}

static int ameba_wifi_disconnect(const struct device *dev)
{
	int ret = 0;
	struct ameba_wifi_runtime *data = dev->data;

	ret = wifi_disconnect();
	net_dhcpv4_stop(ameba_wifi_iface[STA_WLAN_INDEX]);

	data->state = RTK_STA_STOPPED;
	return ret;
}

int ameba_wifi_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct ameba_wifi_runtime *data = dev->data;
	int ret;
	u8 channel, psk_len = 0;
	u8 *psk = NULL;

	net_eth_carrier_on(ameba_wifi_iface[STA_WLAN_INDEX]);

	if (data->state == RTK_STA_CONNECTING) {
		wifi_mgmt_raise_connect_result_event(ameba_wifi_iface[STA_WLAN_INDEX], -1);
		return -EALREADY;
	}

	data->state = RTK_STA_CONNECTING;

	memcpy(data->status.ssid, params->ssid, params->ssid_length);
	data->status.ssid[params->ssid_length] = '\0';

	if (params->channel != WIFI_CHANNEL_ANY) {
		channel = params->channel;
	} else {
		channel = 0;
	}

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		psk = (u8 *)params->psk;
		psk_len = params->psk_length;
	} else if (params->security == WIFI_SECURITY_TYPE_NONE) {
		psk_len = 0;
	} else if (params->security == WIFI_SECURITY_TYPE_SAE) {
		psk = (u8 *)params->psk;
		psk_len = params->psk_length;
	} else {
		LOG_ERR("Authentication method not supported %d", params->security);
		data->state = RTK_STA_STARTED;
		return -EIO;
	}

	ret = wifi_connect_zephyr((u8 *)params->ssid, params->ssid_length, psk, psk_len, channel);
	if (ret) {
		LOG_ERR("Failed to connect to Wi-Fi access point");
		data->state = RTK_STA_STARTED;
		return -EAGAIN;
	}

	ameba_wifi_handle_connect_event();
	LOG_DBG("assoc success");

	return 0;
}

static int ameba_wifi_scan(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb)
{
	struct ameba_wifi_runtime *data = dev->data;
	int ret;

	if (data->scan_cb != NULL) {
		LOG_INF("Scan callback in progress");
		return -EINPROGRESS;
	}

	p_wifi_join_info_free = NULL;
	data->scan_cb = cb;

	ret = wifi_scan_networks_zephyr((u32)ameba_scan_done_cb);
	if (ret) {
		LOG_ERR("Failed to start Wi-Fi scanning");
		return -EAGAIN;
	}

	return 0;
}

static void configure_ap_mode(struct net_if *iface)
{
	struct in_addr ipaddr, netmask, gateway;

	if (net_addr_pton(AF_INET, "192.168.43.1", &ipaddr) < 0) {
		LOG_ERR("Invalid IP address");
		return;
	}

	if (net_addr_pton(AF_INET, "255.255.255.0", &netmask) < 0) {
		LOG_ERR("Invalid netmask");
		return;
	}

	if (net_addr_pton(AF_INET, "192.168.43.1", &gateway) < 0) {
		LOG_ERR("Invalid gateway");
		return;
	}

	net_if_ipv4_addr_add(iface, &ipaddr, NET_ADDR_MANUAL, 0);
	net_if_ipv4_set_netmask_by_addr(iface, &ipaddr, &netmask);
	net_if_ipv4_set_gw(iface, &gateway);
}

static int ameba_wifi_ap_enable(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct ameba_wifi_runtime *data = dev->data;
	int ret;

	configure_ap_mode(ameba_wifi_iface[1]);

	/* Build Wi-Fi configuration for AP mode */
	memcpy(data->status.ssid, params->ssid, params->ssid_length);
	data->status.ssid[params->ssid_length] = '\0';

	if (params->channel == WIFI_CHANNEL_ANY) {
		params->channel = 0;
	}

	if (params->security >= WIFI_SECURITY_TYPE_SAE &&
	    params->security <= WIFI_SECURITY_TYPE_SAE_AUTO) {
		ret = wifi_start_ap_zephyr((u8 *)params->ssid, params->ssid_length,
					   (u8 *)params->psk, params->psk_length, params->channel,
					   1);
	} else {
		ret = wifi_start_ap_zephyr((u8 *)params->ssid, params->ssid_length,
					   (u8 *)params->psk, params->psk_length, params->channel,
					   0);
	}

	if (ret) {
		LOG_ERR("Failed to enable Wi-Fi AP mode");
		return -EAGAIN;
	}

	net_eth_carrier_on(ameba_wifi_iface[1]);
	ameba_data[SOFTAP_WLAN_INDEX].state = RTK_STAAP_STARTED;

	return 0;
}

static int ameba_wifi_ap_disable(const struct device *dev)
{
	(void)dev;

	int ret = wifi_stop_ap();

	net_eth_carrier_off(ameba_wifi_iface[1]);
	ameba_data[SOFTAP_WLAN_INDEX].state = RTK_AP_STOPPED;

	return ret;
}

static int ameba_wifi_status(const struct device *dev, struct wifi_iface_status *status)
{
	struct ameba_wifi_runtime *data = dev->data;
	uint8_t idx = data->if_idx;
	uint32_t security_type;

	if (idx == STA_WLAN_INDEX && data->state >= RTK_STA_CONNECTED) {
		status->state = WIFI_STATE_COMPLETED;
		status->iface_mode = WIFI_MODE_INFRA;
	} else if (idx == SOFTAP_WLAN_INDEX && data->state >= RTK_STAAP_STARTED) {
		status->state = WIFI_SAP_IFACE_ENABLED;
		status->iface_mode = WIFI_MODE_AP;
	} else {
		return 0;
	}

	wifi_get_setting_zephyr(idx, status->ssid, (u8 *)&status->ssid_len, status->bssid,
				&status->channel, &security_type);

	if (status->channel > 11) {
		status->band = WIFI_FREQ_BAND_5_GHZ;
	} else {
		status->band = WIFI_FREQ_BAND_2_4_GHZ;
	}

	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->mfp = WIFI_MFP_DISABLE;

	switch (security_type) {
	case RTW_SECURITY_WPA_AES_PSK:
	case RTW_SECURITY_WPA2_AES_PSK:
		status->security = WIFI_SECURITY_TYPE_PSK;
		break;
	case RTW_SECURITY_WPA3_AES_PSK:
	case RTW_SECURITY_WPA3_OWE:
		status->security = WIFI_SECURITY_TYPE_SAE;
		status->mfp = WIFI_MFP_REQUIRED;
		break;
	case RTW_SECURITY_OPEN:
		status->security = WIFI_SECURITY_TYPE_NONE;
		break;
	default:
		LOG_ERR("unkonwn security type");
		break;
	}
	return 0;
}

static void ameba_wifi_init(struct net_if *iface)
{
	const char *iface_name[2] = {"wlan0", "wlan1"};
	const struct device *dev = net_if_get_device(iface);
	struct ameba_wifi_runtime *dev_data = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	struct wifi_nm_instance *nm = wifi_nm_get_instance("rtk_wifi_nm");

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	ameba_wifi_iface[if_init_idx] = iface;
	dev_data->state = RTK_STA_STOPPED;
	dev_data->if_idx = if_init_idx;
#if !defined(SOC_SERIES_AMEBAD)
	if (settings_subsys_init()) {
		LOG_ERR("setting subsys fail");
	}
#endif

	if (if_init_idx == STA_WLAN_INDEX) {
		wlan_int_enable();
		wifi_init();
	}

	/* Start interface when we are actually connected with Wi-Fi network */
	wifi_get_mac_address(if_init_idx, (struct _rtw_mac_t *)dev_data->mac_addr, 1);

	/* Assign link local address. */
	net_if_set_link_addr(iface, dev_data->mac_addr, 6, NET_LINK_ETHERNET);

	ethernet_init(iface);
	net_if_carrier_off(iface);

#ifndef CONFIG_SINGLE_CORE_WIFI
	ameba_wifi_internal_reg_rxcb(0, eth_rtk_rx);
#endif

	/* separate ap and sta */
	net_if_set_name(iface, iface_name[if_init_idx]);

	nm->mgd_ifaces[if_init_idx].iface = NULL;
	if (if_init_idx == STA_WLAN_INDEX) {
		wifi_nm_register_mgd_type_iface(nm, WIFI_TYPE_STA, iface);
	} else {
		wifi_nm_register_mgd_type_iface(nm, WIFI_TYPE_SAP, iface);
	}

	if_init_idx++;
}

static int ameba_wifi_dev_init(const struct device *dev)
{
	if (dev_init_done != 0) {
		return 0;
	}

	k_tid_t tid =
		k_thread_create(&ameba_wifi_event_thread, ameba_wifi_event_stack,
				WIFI_EVENT_STACK_SIZE, (k_thread_entry_t)ameba_wifi_event_task,
				NULL, NULL, NULL, 14, K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_name_set(tid, "ameba_wifi_event");

	/* add event call back in net_mgmt */
	if (IS_ENABLED(CONFIG_RTK_WIFI_STA_AUTO_DHCPV4)) {
		net_mgmt_init_event_callback(&ameba_dhcp_cb, wifi_event_handler, DHCPV4_MASK);
		net_mgmt_add_event_callback(&ameba_dhcp_cb);
	}

#if defined(CONFIG_BT_COEXIST)
	coex_ipc_entry();
#endif
	dev_init_done++;

	return 0;
}

static const struct wifi_mgmt_ops ameba_wifi_mgmt = {
	.scan = ameba_wifi_scan,
	.connect = ameba_wifi_connect,
	.disconnect = ameba_wifi_disconnect,
	.ap_enable = ameba_wifi_ap_enable,
	.ap_disable = ameba_wifi_ap_disable,
	.iface_status = ameba_wifi_status,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats = ameba_wifi_stats,
#endif
};

static const struct net_wifi_mgmt_offload rtk_api = {
	.wifi_iface.iface_api.init = ameba_wifi_init,
	.wifi_iface.send = ameba_wifi_send,
	.wifi_mgmt_api = &ameba_wifi_mgmt,
};

/* inst replace by DT_DRV_COMPAT(inst) */
NET_DEVICE_DT_INST_DEFINE(0, ameba_wifi_dev_init, NULL, &ameba_data[STA_WLAN_INDEX], NULL,
			  CONFIG_WIFI_INIT_PRIORITY, &rtk_api, ETHERNET_L2,
			  NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

DEFINE_WIFI_NM_INSTANCE(rtk_wifi_nm, &ameba_wifi_mgmt);
CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));

NET_DEVICE_DT_INST_DEFINE(1, ameba_wifi_dev_init, NULL, &ameba_data[SOFTAP_WLAN_INDEX], NULL,
			  CONFIG_WIFI_INIT_PRIORITY, &rtk_api, ETHERNET_L2,
			  NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
