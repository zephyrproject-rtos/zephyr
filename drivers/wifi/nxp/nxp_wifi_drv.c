/**
 * Copyright 2023-2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file nxp_wifi_drv.c
 * Shim layer between wifi driver connection manager and zephyr
 * Wi-Fi L2 layer
 */

#define DT_DRV_COMPAT nxp_wifi

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/device.h>
#include <soc.h>
#include <ethernet/eth_stats.h>
#include <zephyr/logging/log.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#ifdef CONFIG_PM_DEVICE
#include <zephyr/pm/device.h>
#endif
#ifdef CONFIG_WIFI_NM
#include <zephyr/net/wifi_nm.h>
#endif

LOG_MODULE_REGISTER(nxp_wifi, CONFIG_WIFI_LOG_LEVEL);

#include "nxp_wifi_drv.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifdef CONFIG_NXP_WIFI_TC_RELOCATE
#define NXP_WIFI_SET_FUNC_ATTR __ramfunc
#else
#define NXP_WIFI_SET_FUNC_ATTR
#endif

#ifdef CONFIG_RW610
#define IMU_IRQ_N        DT_INST_IRQ_BY_IDX(0, 0, irq)
#define IMU_IRQ_P        DT_INST_IRQ_BY_IDX(0, 0, priority)
#define IMU_WAKEUP_IRQ_N DT_INST_IRQ_BY_IDX(0, 1, irq)
#define IMU_WAKEUP_IRQ_P DT_INST_IRQ_BY_IDX(0, 1, priority)
#endif
/*******************************************************************************
 * Variables
 ******************************************************************************/
static nxp_wifi_state_t s_nxp_wifi_State = NXP_WIFI_NOT_INITIALIZED;
static bool s_nxp_wifi_StaConnected;
static bool s_nxp_wifi_UapActivated;
static struct k_event s_nxp_wifi_SyncEvent;

static struct nxp_wifi_dev nxp_wifi0; /* static instance */

static struct wlan_network nxp_wlan_network;

#ifndef CONFIG_WIFI_NM_HOSTAPD_AP
static char uap_ssid[IEEEtypes_SSID_SIZE + 1];
#endif

extern interface_t g_mlan;
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
extern interface_t g_uap;
#endif
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
extern const rtos_wpa_supp_dev_ops wpa_supp_ops;
#endif

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_RW610)
extern int is_hs_handshake_done;
extern int wlan_host_sleep_state;
#endif

static int nxp_wifi_recv(struct net_if *iface, struct net_pkt *pkt);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#ifdef CONFIG_NXP_WIFI_STA_AUTO_CONN
static void nxp_wifi_auto_connect(void);
#endif

/* Callback Function passed to WLAN Connection Manager. The callback function
 * gets called when there are WLAN Events that need to be handled by the
 * application.
 */
int nxp_wifi_wlan_event_callback(enum wlan_event_reason reason, void *data)
{
	int ret;
#ifndef CONFIG_WIFI_NM_WPA_SUPPLICANT
	struct wlan_ip_config addr;
	char ssid[IEEEtypes_SSID_SIZE + 1];
	char ip[16];
#endif
	static int auth_fail;
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
	wlan_uap_client_disassoc_t *disassoc_resp = data;
	struct in_addr dhcps_addr4;
	struct in_addr base_addr;
	struct in_addr netmask_addr;
#endif
	struct wifi_iface_status status = { 0 };
	struct wifi_ap_sta_info ap_sta_info = { 0 };

	LOG_DBG("WLAN: received event %d", reason);

	if (s_nxp_wifi_State >= NXP_WIFI_INITIALIZED) {
		/* Do not replace the current set of events  */
		k_event_post(&s_nxp_wifi_SyncEvent, NXP_WIFI_EVENT_BIT(reason));
	}

	switch (reason) {
	case WLAN_REASON_INITIALIZED:
		LOG_DBG("WLAN initialized");

#ifdef CONFIG_NET_INTERFACE_NAME
		ret = net_if_set_name(g_mlan.netif, "ml");
		if (ret < 0) {
			LOG_ERR("Failed to set STA nxp_wlan_network interface name");
			return 0;
		}
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
		ret = net_if_set_name(g_uap.netif, "ua");
		if (ret < 0) {
			LOG_ERR("Failed to set uAP nxp_wlan_network interface name");
			return 0;
		}
#endif
#endif

#ifdef CONFIG_NXP_WIFI_STA_AUTO_CONN
		nxp_wifi_auto_connect();
#endif
		break;
	case WLAN_REASON_INITIALIZATION_FAILED:
		LOG_ERR("WLAN: initialization failed");
		break;
	case WLAN_REASON_AUTH_SUCCESS:
		net_eth_carrier_on(g_mlan.netif);
		LOG_DBG("WLAN: authenticated to nxp_wlan_network");
		break;
	case WLAN_REASON_SUCCESS:
		LOG_DBG("WLAN: connected to nxp_wlan_network");
#ifndef CONFIG_WIFI_NM_WPA_SUPPLICANT
		ret = wlan_get_address(&addr);
		if (ret != WM_SUCCESS) {
			LOG_ERR("failed to get IP address");
			return 0;
		}

		net_inet_ntoa(addr.ipv4.address, ip);

		ret = wlan_get_current_network_ssid(ssid);
		if (ret != WM_SUCCESS) {
			LOG_ERR("Failed to get External AP nxp_wlan_network ssid");
			return 0;
		}

		LOG_DBG("Connected to following BSS:");
		LOG_DBG("SSID = [%s]", ssid);
		if (addr.ipv4.address != 0U) {
			LOG_DBG("IPv4 Address: [%s]", ip);
		}
#ifdef CONFIG_NXP_WIFI_IPV6
		ARRAY_FOR_EACH(addr.ipv6, i) {
			if ((addr.ipv6[i].addr_state == NET_ADDR_TENTATIVE) ||
			    (addr.ipv6[i].addr_state == NET_ADDR_PREFERRED)) {
				LOG_DBG("IPv6 Address: %-13s:\t%s (%s)",
					ipv6_addr_type_to_desc(
						(struct net_ipv6_config *)&addr.ipv6[i]),
					ipv6_addr_addr_to_desc(
						(struct net_ipv6_config *)&addr.ipv6[i]),
					ipv6_addr_state_to_desc(addr.ipv6[i].addr_state));
			}
		}
		LOG_DBG("");
#endif
#endif
		auth_fail = 0;
		s_nxp_wifi_StaConnected = true;
		wifi_mgmt_raise_connect_result_event(g_mlan.netif, 0);
		break;
	case WLAN_REASON_CONNECT_FAILED:
		net_eth_carrier_off(g_mlan.netif);
		LOG_WRN("WLAN: connect failed");
		break;
	case WLAN_REASON_NETWORK_NOT_FOUND:
		net_eth_carrier_off(g_mlan.netif);
		LOG_WRN("WLAN: nxp_wlan_network not found");
		break;
	case WLAN_REASON_NETWORK_AUTH_FAILED:
		net_eth_carrier_off(g_mlan.netif);
		LOG_WRN("WLAN: nxp_wlan_network authentication failed");
		auth_fail++;
		if (auth_fail >= 3) {
			LOG_WRN("Authentication Failed. Disconnecting ... ");
			wlan_disconnect();
			auth_fail = 0;
		}
		break;
	case WLAN_REASON_ADDRESS_SUCCESS:
		LOG_DBG("wlan_network mgr: DHCP new lease");
		break;
	case WLAN_REASON_ADDRESS_FAILED:
		LOG_WRN("failed to obtain an IP address");
		break;
	case WLAN_REASON_USER_DISCONNECT:
		net_eth_carrier_off(g_mlan.netif);
		LOG_DBG("disconnected");
		auth_fail = 0;
		s_nxp_wifi_StaConnected = false;
		wifi_mgmt_raise_disconnect_result_event(g_mlan.netif, 0);
		break;
	case WLAN_REASON_LINK_LOST:
		net_eth_carrier_off(g_mlan.netif);
		LOG_WRN("WLAN: link lost");
		break;
	case WLAN_REASON_CHAN_SWITCH:
		LOG_DBG("WLAN: channel switch");
		break;
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
	case WLAN_REASON_UAP_SUCCESS:
		net_eth_carrier_on(g_uap.netif);
		LOG_DBG("WLAN: UAP Started");
#ifndef CONFIG_WIFI_NM_HOSTAPD_AP
		ret = wlan_get_current_uap_network_ssid(uap_ssid);
		if (ret != WM_SUCCESS) {
			LOG_ERR("Failed to get Soft AP nxp_wlan_network ssid");
			return 0;
		}

		LOG_DBG("Soft AP \"%s\" started successfully", uap_ssid);
#endif
		if (net_addr_pton(AF_INET, CONFIG_NXP_WIFI_SOFTAP_IP_ADDRESS, &dhcps_addr4) < 0) {
			LOG_ERR("Invalid CONFIG_NXP_WIFI_SOFTAP_IP_ADDRESS");
			return 0;
		}
		net_if_ipv4_addr_add(g_uap.netif, &dhcps_addr4, NET_ADDR_MANUAL, 0);
		net_if_ipv4_set_gw(g_uap.netif, &dhcps_addr4);

		if (net_addr_pton(AF_INET, CONFIG_NXP_WIFI_SOFTAP_IP_MASK, &netmask_addr) < 0) {
			LOG_ERR("Invalid CONFIG_NXP_WIFI_SOFTAP_IP_MASK");
			return 0;
		}
		net_if_ipv4_set_netmask_by_addr(g_uap.netif, &dhcps_addr4, &netmask_addr);
		net_if_up(g_uap.netif);

		if (net_addr_pton(AF_INET, CONFIG_NXP_WIFI_SOFTAP_IP_BASE, &base_addr) < 0) {
			LOG_ERR("Invalid CONFIG_NXP_WIFI_SOFTAP_IP_BASE");
			return 0;
		}

		if (net_dhcpv4_server_start(g_uap.netif, &base_addr) < 0) {
			LOG_ERR("DHCP Server start failed");
			return 0;
		}

		LOG_DBG("DHCP Server started successfully");
		s_nxp_wifi_UapActivated = true;
		break;
	case WLAN_REASON_UAP_CLIENT_ASSOC:
		LOG_DBG("WLAN: UAP a Client Associated");
		LOG_DBG("Client => ");
		print_mac((const char *)data);
		LOG_DBG("Associated with Soft AP");
		break;
	case WLAN_REASON_UAP_CLIENT_CONN:
		wlan_get_current_uap_network(&nxp_wlan_network);
#ifdef CONFIG_NXP_WIFI_11AX
			if (nxp_wlan_network.dot11ax) {
				ap_sta_info.link_mode = WIFI_6;
			}
#endif
#ifdef CONFIG_NXP_WIFI_11AC
			else if (nxp_wlan_network.dot11ac) {
				ap_sta_info.link_mode = WIFI_5;
			}
#endif
			else if (nxp_wlan_network.dot11n) {
				ap_sta_info.link_mode = WIFI_4;
			} else {
				ap_sta_info.link_mode = WIFI_3;
			}
		
		memcpy(ap_sta_info.mac, data, WIFI_MAC_ADDR_LEN);
		ap_sta_info.mac_length  = WIFI_MAC_ADDR_LEN;
		ap_sta_info.twt_capable = status.twt_capable;

		wifi_mgmt_raise_ap_sta_connected_event(g_uap.netif, &ap_sta_info);
		LOG_DBG("WLAN: UAP a Client Connected");
		LOG_DBG("Client => ");
		print_mac((const char *)data);
		LOG_DBG("Connected with Soft AP");
		break;
	case WLAN_REASON_UAP_CLIENT_DISSOC:
		memcpy(ap_sta_info.mac, disassoc_resp->sta_addr, WIFI_MAC_ADDR_LEN);
		wifi_mgmt_raise_ap_sta_disconnected_event(g_uap.netif, &ap_sta_info);
		
		LOG_DBG("WLAN: UAP a Client Dissociated:");
		LOG_DBG(" Client MAC => ");
		print_mac((const char *)(disassoc_resp->sta_addr));
		LOG_DBG(" Reason code => ");
		LOG_DBG("%d", disassoc_resp->reason_code);
		break;
	case WLAN_REASON_UAP_STOPPED:
		net_eth_carrier_off(g_uap.netif);
		LOG_DBG("WLAN: UAP Stopped");

		net_dhcpv4_server_stop(g_uap.netif);
		LOG_DBG("DHCP Server stopped successfully");
		s_nxp_wifi_UapActivated = false;
		break;
#endif
	case WLAN_REASON_PS_ENTER:
		LOG_DBG("WLAN: PS_ENTER");
		break;
	case WLAN_REASON_PS_EXIT:
		LOG_DBG("WLAN: PS EXIT");
		break;
#ifdef CONFIG_NXP_WIFI_SUBSCRIBE_EVENT_SUPPORT
	case WLAN_REASON_RSSI_HIGH:
	case WLAN_REASON_SNR_LOW:
	case WLAN_REASON_SNR_HIGH:
	case WLAN_REASON_MAX_FAIL:
	case WLAN_REASON_BEACON_MISSED:
	case WLAN_REASON_DATA_RSSI_LOW:
	case WLAN_REASON_DATA_RSSI_HIGH:
	case WLAN_REASON_DATA_SNR_LOW:
	case WLAN_REASON_DATA_SNR_HIGH:
	case WLAN_REASON_LINK_QUALITY:
	case WLAN_REASON_PRE_BEACON_LOST:
		break;
#endif
	default:
		LOG_WRN("WLAN: Unknown Event: %d", reason);
	}
	return 0;
}

static int nxp_wifi_wlan_init(void)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret;

	if (s_nxp_wifi_State != NXP_WIFI_NOT_INITIALIZED) {
		status = NXP_WIFI_RET_FAIL;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		k_event_init(&s_nxp_wifi_SyncEvent);
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		ret = wlan_init(wlan_fw_bin, wlan_fw_bin_len);
		if (ret != WM_SUCCESS) {
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		s_nxp_wifi_State = NXP_WIFI_INITIALIZED;
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		return -1;
	}

	nxp_wifi_internal_register_rx_cb(nxp_wifi_recv);

	return 0;
}

static int nxp_wifi_wlan_start(void)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret;
	uint32_t syncBit;

	if (s_nxp_wifi_State != NXP_WIFI_INITIALIZED) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		k_event_clear(&s_nxp_wifi_SyncEvent, NXP_WIFI_SYNC_INIT_GROUP);

		ret = wlan_start(nxp_wifi_wlan_event_callback);
		if (ret != WM_SUCCESS) {
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		syncBit = k_event_wait(&s_nxp_wifi_SyncEvent, NXP_WIFI_SYNC_INIT_GROUP, false,
				       NXP_WIFI_SYNC_TIMEOUT_MS);
		k_event_clear(&s_nxp_wifi_SyncEvent, NXP_WIFI_SYNC_INIT_GROUP);
		if (syncBit & NXP_WIFI_EVENT_BIT(WLAN_REASON_INITIALIZED)) {
			status = NXP_WIFI_RET_SUCCESS;
		} else if (syncBit & NXP_WIFI_EVENT_BIT(WLAN_REASON_INITIALIZATION_FAILED)) {
			status = NXP_WIFI_RET_FAIL;
		} else {
			status = NXP_WIFI_RET_TIMEOUT;
		}
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		s_nxp_wifi_State = NXP_WIFI_STARTED;
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		return -1;
	}

	return 0;
}

#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT

static int nxp_wifi_start_ap(const struct device *dev, struct wifi_connect_req_params *params)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret;
	interface_t *if_handle = (interface_t *)&g_uap;

	if (if_handle->state.interface != WLAN_BSS_TYPE_UAP) {
		LOG_ERR("Wi-Fi not in uAP mode");
		return -EIO;
	}

	if ((s_nxp_wifi_State != NXP_WIFI_STARTED) || (s_nxp_wifi_UapActivated != false)) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		if ((params->ssid_length == 0) || (params->ssid_length > IEEEtypes_SSID_SIZE)) {
			status = NXP_WIFI_RET_BAD_PARAM;
		}
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		wlan_remove_network(nxp_wlan_network.name);

		wlan_initialize_uap_network(&nxp_wlan_network);

		memcpy(nxp_wlan_network.name, NXP_WIFI_UAP_NETWORK_NAME,
		       strlen(NXP_WIFI_UAP_NETWORK_NAME));

		memcpy(nxp_wlan_network.ssid, params->ssid, params->ssid_length);

		if (params->channel == WIFI_CHANNEL_ANY) {
			nxp_wlan_network.channel = 0;
		} else {
			nxp_wlan_network.channel = params->channel;
		}

		if (params->mfp == WIFI_MFP_REQUIRED) {
			nxp_wlan_network.security.mfpc = true;
			nxp_wlan_network.security.mfpr = true;
		} else if (params->mfp == WIFI_MFP_OPTIONAL) {
			nxp_wlan_network.security.mfpc = true;
			nxp_wlan_network.security.mfpr = false;
		}

		if (params->security == WIFI_SECURITY_TYPE_NONE) {
			nxp_wlan_network.security.type = WLAN_SECURITY_NONE;
		} else if (params->security == WIFI_SECURITY_TYPE_PSK) {
			nxp_wlan_network.security.type = WLAN_SECURITY_WPA2;
			nxp_wlan_network.security.psk_len = params->psk_length;
			strncpy(nxp_wlan_network.security.psk, params->psk, params->psk_length);
		}
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
		else if (params->security == WIFI_SECURITY_TYPE_PSK_SHA256) {
			nxp_wlan_network.security.type = WLAN_SECURITY_WPA2;
			nxp_wlan_network.security.key_mgmt |= WLAN_KEY_MGMT_PSK_SHA256;
			nxp_wlan_network.security.psk_len = params->psk_length;
			strncpy(nxp_wlan_network.security.psk, params->psk, params->psk_length);
		}
#endif
		else if (params->security == WIFI_SECURITY_TYPE_SAE) {
			nxp_wlan_network.security.type = WLAN_SECURITY_WPA3_SAE;
			nxp_wlan_network.security.password_len = params->psk_length;
			strncpy(nxp_wlan_network.security.password, params->psk,
				params->psk_length);
		} else {
			status = NXP_WIFI_RET_BAD_PARAM;
		}
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		ret = wlan_add_network(&nxp_wlan_network);
		if (ret != WM_SUCCESS) {
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		ret = wlan_start_network(nxp_wlan_network.name);
		if (ret != WM_SUCCESS) {
			wlan_remove_network(nxp_wlan_network.name);
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		LOG_ERR("Failed to enable Wi-Fi AP mode");
		return -EAGAIN;
	}

	return 0;
}

static int nxp_wifi_stop_ap(const struct device *dev)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret;
	interface_t *if_handle = (interface_t *)&g_uap;

	if (if_handle->state.interface != WLAN_BSS_TYPE_UAP) {
		LOG_ERR("Wi-Fi not in uAP mode");
		return -EIO;
	}

	if ((s_nxp_wifi_State != NXP_WIFI_STARTED) || (s_nxp_wifi_UapActivated != true)) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		ret = wlan_stop_network(NXP_WIFI_UAP_NETWORK_NAME);
		if (ret != WM_SUCCESS) {
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		LOG_ERR("Failed to disable Wi-Fi AP mode");
		return -EAGAIN;
	}

	return 0;
}

static int nxp_wifi_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret = WM_SUCCESS;
	interface_t *if_handle = (interface_t *)dev->data;

	if (if_handle->state.interface != WLAN_BSS_TYPE_UAP) {
		LOG_ERR("Wi-Fi not in uAP mode");
		return -EIO;
	}

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		if (params->type & WIFI_AP_CONFIG_PARAM_MAX_INACTIVITY) {
			ret = wlan_uap_set_sta_ageout_timer(params->max_inactivity * 10);
			if (ret != WM_SUCCESS) {
				status = NXP_WIFI_RET_FAIL;
			}
		} else {
			return -EINVAL;
		}
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		return -EAGAIN;
	}

	return 0;
}
#endif

static int nxp_wifi_process_results(unsigned int count)
{
	struct wlan_scan_result scan_result = {0};
	struct wifi_scan_result res = {0};

	if (!count) {
		LOG_DBG("No Wi-Fi AP found");
		goto out;
	}

	if (g_mlan.max_bss_cnt) {
		count = g_mlan.max_bss_cnt > count ? count : g_mlan.max_bss_cnt;
	}

	for (int i = 0; i < count; i++) {
		wlan_get_scan_result(i, &scan_result);

		memset(&res, 0, sizeof(struct wifi_scan_result));

		memcpy(res.mac, scan_result.bssid, WIFI_MAC_ADDR_LEN);
		res.mac_length = WIFI_MAC_ADDR_LEN;
		res.ssid_length = scan_result.ssid_len;
		strncpy(res.ssid, scan_result.ssid, scan_result.ssid_len);

		res.rssi = -scan_result.rssi;
		res.channel = scan_result.channel;
		res.band = scan_result.channel > 14 ? WIFI_FREQ_BAND_5_GHZ : WIFI_FREQ_BAND_2_4_GHZ;

		res.security = WIFI_SECURITY_TYPE_NONE;

		if (scan_result.wpa2_entp) {
			res.security = WIFI_SECURITY_TYPE_EAP_TLS;
		}
		if (scan_result.wpa2) {
			res.security = WIFI_SECURITY_TYPE_PSK;
		}
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
		if (scan_result.wpa2_sha256) {
			res.security = WIFI_SECURITY_TYPE_PSK_SHA256;
		}
#endif
		if (scan_result.wpa3_sae) {
			res.security = WIFI_SECURITY_TYPE_SAE;
		}

		if (scan_result.ap_mfpr) {
			res.mfp = WIFI_MFP_REQUIRED;
		} else if (scan_result.ap_mfpc) {
			res.mfp = WIFI_MFP_OPTIONAL;
		}

		if (g_mlan.scan_cb) {
			g_mlan.scan_cb(g_mlan.netif, 0, &res);

			/* ensure notifications get delivered */
			k_yield();
		}
	}

out:
	/* report end of scan event */
	if (g_mlan.scan_cb) {
		g_mlan.scan_cb(g_mlan.netif, 0, NULL);
	}

	g_mlan.scan_cb = NULL;

	return WM_SUCCESS;
}

static int nxp_wifi_scan(const struct device *dev, struct wifi_scan_params *params,
			 scan_result_cb_t cb)
{
	int ret;
	interface_t *if_handle = (interface_t *)dev->data;
	wlan_scan_params_v2_t wlan_scan_params_v2 = {0};
	uint8_t i = 0;

	if (if_handle->state.interface != WLAN_BSS_TYPE_STA) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EIO;
	}

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		LOG_ERR("Wi-Fi not started status %d", s_nxp_wifi_State);
		return -EBUSY;
	}

	if (g_mlan.scan_cb != NULL) {
		LOG_WRN("Scan callback in progress");
		return -EINPROGRESS;
	}

	g_mlan.scan_cb = cb;

	if (params == NULL) {
		goto do_scan;
	}

	g_mlan.max_bss_cnt = params->max_bss_cnt;

	if (params->bands & (1 << WIFI_FREQ_BAND_6_GHZ)) {
		LOG_ERR("Wi-Fi band 6 GHz not supported");
		g_mlan.scan_cb = NULL;
		return -EIO;
	}
#if (CONFIG_WIFI_MGMT_SCAN_SSID_FILT_MAX > 0)
	if (params->ssids[0]) {
		strncpy(wlan_scan_params_v2.ssid[0], params->ssids[0], WIFI_SSID_MAX_LEN);
		wlan_scan_params_v2.ssid[0][WIFI_SSID_MAX_LEN - 1] = 0;
	}

#if (CONFIG_WIFI_MGMT_SCAN_SSID_FILT_MAX > 1)
	if (params->ssids[1]) {
		strncpy(wlan_scan_params_v2.ssid[1], params->ssids[1], WIFI_SSID_MAX_LEN);
		wlan_scan_params_v2.ssid[1][WIFI_SSID_MAX_LEN - 1] = 0;
	}
#endif
#endif

#ifdef CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL

	for (i = 0; i < WIFI_MGMT_SCAN_CHAN_MAX_MANUAL && params->band_chan[i].channel; i++) {
		if (params->scan_type == WIFI_SCAN_TYPE_ACTIVE) {
			wlan_scan_params_v2.chan_list[i].scan_type = MLAN_SCAN_TYPE_ACTIVE;
			wlan_scan_params_v2.chan_list[i].scan_time = params->dwell_time_active;
		} else {
			wlan_scan_params_v2.chan_list[i].scan_type = MLAN_SCAN_TYPE_PASSIVE;
			wlan_scan_params_v2.chan_list[i].scan_time = params->dwell_time_passive;
		}

		wlan_scan_params_v2.chan_list[i].chan_number =
			(uint8_t)(params->band_chan[i].channel);
	}
#endif

	wlan_scan_params_v2.num_channels = i;

	if (params->bands & (1 << WIFI_FREQ_BAND_2_4_GHZ)) {
		wlan_scan_params_v2.chan_list[0].radio_type = 0 | BAND_SPECIFIED;
	}
#ifdef CONFIG_5GHz_SUPPORT
	if (params->bands & (1 << WIFI_FREQ_BAND_5_GHZ)) {
		if (wlan_scan_params_v2.chan_list[0].radio_type & BAND_SPECIFIED) {
			wlan_scan_params_v2.chan_list[0].radio_type = 0;
		} else {
			wlan_scan_params_v2.chan_list[0].radio_type = 1 | BAND_SPECIFIED;
		}
	}
#else
	if (params->bands & (1 << WIFI_FREQ_BAND_5_GHZ)) {
		LOG_ERR("Wi-Fi band 5Hz not supported");
		g_mlan.scan_cb = NULL;
		return -EIO;
	}
#endif

do_scan:
	wlan_scan_params_v2.cb = nxp_wifi_process_results;

	ret = wlan_scan_with_opt(wlan_scan_params_v2);
	if (ret != WM_SUCCESS) {
		LOG_ERR("Failed to start Wi-Fi scanning");
		g_mlan.scan_cb = NULL;
		return -EAGAIN;
	}

	return 0;
}

static int nxp_wifi_version(const struct device *dev, struct wifi_version *params)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		LOG_ERR("Failed to get Wi-Fi driver/firmware version");
		return -EAGAIN;
	}

	params->drv_version = WLAN_DRV_VERSION;

	params->fw_version = wlan_get_firmware_version_ext();

	return 0;
}

static int nxp_wifi_ap_bandwidth(const struct device *dev, struct wifi_ap_config_params *params)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret = WM_SUCCESS;
	interface_t *if_handle = (interface_t *)dev->data;

	if (if_handle->state.interface != WLAN_BSS_TYPE_UAP) {
		LOG_ERR("Wi-Fi not in uAP mode");
		return -EIO;
	}

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {

		if (params->oper == WIFI_MGMT_SET) {

			ret = wlan_uap_set_bandwidth(params->bandwidth);

			if (ret != WM_SUCCESS) {
				status = NXP_WIFI_RET_FAIL;
			}
		} else {
			ret = wlan_uap_get_bandwidth(&params->bandwidth);

			if (ret != WM_SUCCESS) {
				status = NXP_WIFI_RET_FAIL;
			}
		}
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		LOG_ERR("Failed to get/set Wi-Fi AP bandwidth");
		return -EAGAIN;
	}

	return 0;
}

static int nxp_wifi_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret;
	interface_t *if_handle = (interface_t *)dev->data;

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		LOG_ERR("Wi-Fi not started");
		wifi_mgmt_raise_connect_result_event(g_mlan.netif, -1);
		return -EALREADY;
	}

	if (if_handle->state.interface != WLAN_BSS_TYPE_STA) {
		LOG_ERR("Wi-Fi not in station mode");
		wifi_mgmt_raise_connect_result_event(g_mlan.netif, -1);
		return -EIO;
	}

	if ((params->ssid_length == 0) || (params->ssid_length > IEEEtypes_SSID_SIZE)) {
		status = NXP_WIFI_RET_BAD_PARAM;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		wlan_disconnect();

		wlan_remove_network(nxp_wlan_network.name);

		wlan_initialize_sta_network(&nxp_wlan_network);

		memcpy(nxp_wlan_network.name, NXP_WIFI_STA_NETWORK_NAME,
		       strlen(NXP_WIFI_STA_NETWORK_NAME));

		memcpy(nxp_wlan_network.ssid, params->ssid, params->ssid_length);

		nxp_wlan_network.ssid_specific = 1;

		if (params->channel == WIFI_CHANNEL_ANY) {
			nxp_wlan_network.channel = 0;
		} else {
			nxp_wlan_network.channel = params->channel;
		}

#ifdef CONFIG_NXP_WIFI_IP_STATIC
		nxp_wlan_network.ip.ipv4.addr_type = NET_ADDR_TYPE_STATIC;

		nxp_wlan_network.ip.ipv4.address = net_inet_aton(CONFIG_NXP_WIFI_IP_ADDRESS);
		nxp_wlan_network.ip.ipv4.netmask = net_inet_aton(CONFIG_NXP_WIFI_IP_MASK);
		nxp_wlan_network.ip.ipv4.gw = net_inet_aton(CONFIG_NXP_WIFI_IP_GATEWAY);
#endif

		if (params->mfp == WIFI_MFP_REQUIRED) {
			nxp_wlan_network.security.mfpc = true;
			nxp_wlan_network.security.mfpr = true;
		} else if (params->mfp == WIFI_MFP_OPTIONAL) {
			nxp_wlan_network.security.mfpc = true;
			nxp_wlan_network.security.mfpr = false;
		}

		if (params->security == WIFI_SECURITY_TYPE_NONE) {
			nxp_wlan_network.security.type = WLAN_SECURITY_NONE;
		} else if (params->security == WIFI_SECURITY_TYPE_PSK) {
			nxp_wlan_network.security.type = WLAN_SECURITY_WPA2;
			nxp_wlan_network.security.psk_len = params->psk_length;
			strncpy(nxp_wlan_network.security.psk, params->psk, params->psk_length);
		}
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
		else if (params->security == WIFI_SECURITY_TYPE_PSK_SHA256) {
			nxp_wlan_network.security.type = WLAN_SECURITY_WPA2;
			nxp_wlan_network.security.key_mgmt |= WLAN_KEY_MGMT_PSK_SHA256;
			nxp_wlan_network.security.psk_len = params->psk_length;
			strncpy(nxp_wlan_network.security.psk, params->psk, params->psk_length);
		}
#endif
		else if (params->security == WIFI_SECURITY_TYPE_SAE) {
			nxp_wlan_network.security.type = WLAN_SECURITY_WPA3_SAE;
			nxp_wlan_network.security.password_len = params->psk_length;
			strncpy(nxp_wlan_network.security.password, params->psk,
				params->psk_length);
		} else {
			status = NXP_WIFI_RET_BAD_PARAM;
		}
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		ret = wlan_add_network(&nxp_wlan_network);
		if (ret != WM_SUCCESS) {
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		ret = wlan_connect(nxp_wlan_network.name);
		if (ret != WM_SUCCESS) {
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		LOG_ERR("Failed to connect to Wi-Fi access point");
		return -EAGAIN;
	}

	return 0;
}

static int nxp_wifi_disconnect(const struct device *dev)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret;
	interface_t *if_handle = (interface_t *)dev->data;
	enum wlan_connection_state connection_state = WLAN_DISCONNECTED;

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (if_handle->state.interface != WLAN_BSS_TYPE_STA) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EIO;
	}

	wlan_get_connection_state(&connection_state);
	if (connection_state == WLAN_DISCONNECTED) {
		s_nxp_wifi_StaConnected = false;
		wifi_mgmt_raise_disconnect_result_event(g_mlan.netif, -1);
		return NXP_WIFI_RET_SUCCESS;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		ret = wlan_disconnect();
		if (ret != WM_SUCCESS) {
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		LOG_ERR("Failed to disconnect from AP");
		wifi_mgmt_raise_disconnect_result_event(g_mlan.netif, -1);
		return -EAGAIN;
	}

	wifi_mgmt_raise_disconnect_result_event(g_mlan.netif, 0);

	return 0;
}

static inline enum wifi_security_type nxp_wifi_security_type(enum wlan_security_type type)
{
	switch (type) {
	case WLAN_SECURITY_NONE:
		return WIFI_SECURITY_TYPE_NONE;
	case WLAN_SECURITY_WPA2:
		return WIFI_SECURITY_TYPE_PSK;
	case WLAN_SECURITY_WPA3_SAE:
		return WIFI_SECURITY_TYPE_SAE;
	default:
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}

#ifndef CONFIG_WIFI_NM_WPA_SUPPLICANT
static int nxp_wifi_uap_status(const struct device *dev, struct wifi_iface_status *status)
{
	enum wlan_connection_state connection_state = WLAN_DISCONNECTED;
	interface_t *if_handle = (interface_t *)&g_uap;

	wlan_get_uap_connection_state(&connection_state);

	if (connection_state == WLAN_UAP_STARTED) {

		if (!wlan_get_current_uap_network(&nxp_wlan_network)) {
			strncpy(status->ssid, nxp_wlan_network.ssid, WIFI_SSID_MAX_LEN);
			status->ssid[WIFI_SSID_MAX_LEN - 1] = 0;
			status->ssid_len = strlen(status->ssid);

			memcpy(status->bssid, nxp_wlan_network.bssid, WIFI_MAC_ADDR_LEN);

			status->rssi = nxp_wlan_network.rssi;

			status->channel = nxp_wlan_network.channel;

			status->beacon_interval = nxp_wlan_network.beacon_period;

			status->dtim_period = nxp_wlan_network.dtim_period;

			if (if_handle->state.interface == WLAN_BSS_TYPE_STA) {
				status->iface_mode = WIFI_MODE_INFRA;
			}
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
			else if (if_handle->state.interface == WLAN_BSS_TYPE_UAP) {
				status->iface_mode = WIFI_MODE_AP;
			}
#endif

#ifdef CONFIG_NXP_WIFI_11AX
			if (nxp_wlan_network.dot11ax) {
				status->link_mode = WIFI_6;
			}
#endif
#ifdef CONFIG_NXP_WIFI_11AC
			else if (nxp_wlan_network.dot11ac) {
				status->link_mode = WIFI_5;
			}
#endif
			else if (nxp_wlan_network.dot11n) {
				status->link_mode = WIFI_4;
			} else {
				status->link_mode = WIFI_3;
			}

			status->band = nxp_wlan_network.channel > 14 ? WIFI_FREQ_BAND_5_GHZ
								     : WIFI_FREQ_BAND_2_4_GHZ;
			status->security = nxp_wifi_security_type(nxp_wlan_network.security.type);
			status->mfp = nxp_wlan_network.security.mfpr ? WIFI_MFP_REQUIRED :
				(nxp_wlan_network.security.mfpc ? WIFI_MFP_OPTIONAL : 0);
		}
	}

	return 0;
}
#endif

static int nxp_wifi_status(const struct device *dev, struct wifi_iface_status *status)
{
	enum wlan_connection_state connection_state = WLAN_DISCONNECTED;
	interface_t *if_handle = (interface_t *)dev->data;

	wlan_get_connection_state(&connection_state);

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		status->state = WIFI_STATE_INTERFACE_DISABLED;

		return 0;
	}

	if (connection_state == WLAN_DISCONNECTED) {
		status->state = WIFI_STATE_DISCONNECTED;
	} else if (connection_state == WLAN_SCANNING) {
		status->state = WIFI_STATE_SCANNING;
	} else if (connection_state == WLAN_ASSOCIATING) {
		status->state = WIFI_STATE_ASSOCIATING;
	} else if (connection_state == WLAN_ASSOCIATED) {
		status->state = WIFI_STATE_ASSOCIATED;
#ifdef CONFIG_NXP_WIFI_STA_AUTO_DHCPV4
	} else if (connection_state == WLAN_CONNECTED) {
		status->state = WIFI_STATE_COMPLETED;
#else
	} else if (connection_state == WLAN_AUTHENTICATED) {
		status->state = WIFI_STATE_COMPLETED;
#endif

		if (!wlan_get_current_network(&nxp_wlan_network)) {
			strncpy(status->ssid, nxp_wlan_network.ssid, WIFI_SSID_MAX_LEN - 1);
			status->ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
			status->ssid_len = strlen(nxp_wlan_network.ssid);

			memcpy(status->bssid, nxp_wlan_network.bssid, WIFI_MAC_ADDR_LEN);

			status->rssi = nxp_wlan_network.rssi;

			status->channel = nxp_wlan_network.channel;

			status->beacon_interval = nxp_wlan_network.beacon_period;

			status->dtim_period = nxp_wlan_network.dtim_period;

			if (if_handle->state.interface == WLAN_BSS_TYPE_STA) {
				status->iface_mode = WIFI_MODE_INFRA;
			}
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
			else if (if_handle->state.interface == WLAN_BSS_TYPE_UAP) {
				status->iface_mode = WIFI_MODE_AP;
			}
#endif

#ifdef CONFIG_NXP_WIFI_11AX
			if (nxp_wlan_network.dot11ax) {
				status->link_mode = WIFI_6;
			}
#endif
#ifdef CONFIG_NXP_WIFI_11AC
			else if (nxp_wlan_network.dot11ac) {
				status->link_mode = WIFI_5;
			}
#endif
			else if (nxp_wlan_network.dot11n) {
				status->link_mode = WIFI_4;
			} else {
				status->link_mode = WIFI_3;
			}

			status->band = nxp_wlan_network.channel > 14 ? WIFI_FREQ_BAND_5_GHZ
								     : WIFI_FREQ_BAND_2_4_GHZ;
			status->security = nxp_wifi_security_type(nxp_wlan_network.security.type);
			status->mfp = nxp_wlan_network.security.mfpr ? WIFI_MFP_REQUIRED :
				(nxp_wlan_network.security.mfpc ? WIFI_MFP_OPTIONAL : 0);
		}
	}

	return 0;
}

#if defined(CONFIG_NET_STATISTICS_WIFI)
static int nxp_wifi_stats(const struct device *dev, struct net_stats_wifi *stats)
{
	interface_t *if_handle = (interface_t *)dev->data;

	stats->bytes.received = if_handle->stats.bytes.received;
	stats->bytes.sent = if_handle->stats.bytes.sent;
	stats->pkts.rx = if_handle->stats.pkts.rx;
	stats->pkts.tx = if_handle->stats.pkts.tx;
	stats->errors.rx = if_handle->stats.errors.rx;
	stats->errors.tx = if_handle->stats.errors.tx;
	stats->broadcast.rx = if_handle->stats.broadcast.rx;
	stats->broadcast.tx = if_handle->stats.broadcast.tx;
	stats->multicast.rx = if_handle->stats.multicast.rx;
	stats->multicast.tx = if_handle->stats.multicast.tx;
	stats->sta_mgmt.beacons_rx = if_handle->stats.sta_mgmt.beacons_rx;
	stats->sta_mgmt.beacons_miss = if_handle->stats.sta_mgmt.beacons_miss;

	return 0;
}
#endif

#ifdef CONFIG_NXP_WIFI_STA_AUTO_CONN
static void nxp_wifi_auto_connect(void)
{
	int ssid_len = strlen(CONFIG_NXP_WIFI_STA_AUTO_SSID);
	int psk_len = strlen(CONFIG_NXP_WIFI_STA_AUTO_PASSWORD);
	struct wifi_connect_req_params params = {0};
	char ssid[IEEEtypes_SSID_SIZE] = {0};
	char psk[WLAN_PSK_MAX_LENGTH] = {0};

	if (ssid_len >= IEEEtypes_SSID_SIZE) {
		LOG_ERR("AutoConnect SSID too long");
		return;
	}
	if (ssid_len == 0) {
		LOG_ERR("AutoConnect SSID NULL");
		return;
	}

	strcpy(ssid, CONFIG_NXP_WIFI_STA_AUTO_SSID);

	if (psk_len == 0) {
		params.security = WIFI_SECURITY_TYPE_NONE;
	} else if (psk_len >= WLAN_PSK_MIN_LENGTH && psk_len < WLAN_PSK_MAX_LENGTH) {
		strcpy(psk, CONFIG_NXP_WIFI_STA_AUTO_PASSWORD);
		params.security = WIFI_SECURITY_TYPE_PSK;
	} else {
		LOG_ERR("AutoConnect invalid password length %d", psk_len);
		return;
	}

	params.channel = WIFI_CHANNEL_ANY;
	params.ssid = &ssid[0];
	params.ssid_length = ssid_len;
	params.psk = &psk[0];
	params.psk_length = psk_len;

	LOG_DBG("AutoConnect SSID[%s]", ssid);
	nxp_wifi_connect(g_mlan.netif->if_dev->dev, &params);
}
#endif

static int nxp_wifi_11k_enable(const struct device *dev, struct wifi_11k_params *params)
{
#if CONFIG_11K
	wlan_host_11k_cfg(params->enable_11k);
#endif

	return 0;
}
static int nxp_wifi_power_save(const struct device *dev, struct wifi_ps_params *params)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	int ret = WM_SUCCESS;
	uint32_t syncBit;
	interface_t *if_handle = (interface_t *)dev->data;

	if (if_handle->state.interface != WLAN_BSS_TYPE_STA) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EIO;
	}

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {

		switch (params->type) {
		case WIFI_PS_PARAM_STATE:
			if (params->enabled == WIFI_PS_DISABLED) {
				k_event_clear(&s_nxp_wifi_SyncEvent, NXP_WIFI_SYNC_PS_GROUP);
				ret = wlan_deepsleepps_off();
				if (ret != WM_SUCCESS) {
					status = NXP_WIFI_RET_FAIL;
				}

				if (status == NXP_WIFI_RET_SUCCESS) {
					syncBit = k_event_wait(&s_nxp_wifi_SyncEvent,
							       NXP_WIFI_SYNC_PS_GROUP, false,
							       NXP_WIFI_SYNC_TIMEOUT_MS);
					k_event_clear(&s_nxp_wifi_SyncEvent,
						      NXP_WIFI_SYNC_PS_GROUP);
					if (syncBit & NXP_WIFI_EVENT_BIT(WLAN_REASON_PS_EXIT)) {
						status = NXP_WIFI_RET_SUCCESS;
					} else {
						status = NXP_WIFI_RET_TIMEOUT;
					}
				}

				if (status != NXP_WIFI_RET_SUCCESS) {
					LOG_DBG("Wi-Fi power save is already disabled");
					return -EAGAIN;
				}

				k_event_clear(&s_nxp_wifi_SyncEvent, NXP_WIFI_SYNC_PS_GROUP);
				ret = wlan_ieeeps_off();
				if (ret != WM_SUCCESS) {
					status = NXP_WIFI_RET_FAIL;
				}

				if (status == NXP_WIFI_RET_SUCCESS) {
					syncBit = k_event_wait(&s_nxp_wifi_SyncEvent,
							       NXP_WIFI_SYNC_PS_GROUP, false,
							       NXP_WIFI_SYNC_TIMEOUT_MS);
					k_event_clear(&s_nxp_wifi_SyncEvent,
						      NXP_WIFI_SYNC_PS_GROUP);
					if (syncBit & NXP_WIFI_EVENT_BIT(WLAN_REASON_PS_EXIT)) {
						status = NXP_WIFI_RET_SUCCESS;
					} else {
						status = NXP_WIFI_RET_TIMEOUT;
					}
				}

				if (status != NXP_WIFI_RET_SUCCESS) {
					LOG_DBG("Wi-Fi power save is already disabled");
					return -EAGAIN;
				}
			} else if (params->enabled == WIFI_PS_ENABLED) {
				k_event_clear(&s_nxp_wifi_SyncEvent, NXP_WIFI_SYNC_PS_GROUP);
				ret = wlan_deepsleepps_on();
				if (ret != WM_SUCCESS) {
					status = NXP_WIFI_RET_FAIL;
				}

				if (status == NXP_WIFI_RET_SUCCESS) {
					syncBit = k_event_wait(&s_nxp_wifi_SyncEvent,
							       NXP_WIFI_SYNC_PS_GROUP, false,
							       NXP_WIFI_SYNC_TIMEOUT_MS);
					k_event_clear(&s_nxp_wifi_SyncEvent,
						      NXP_WIFI_SYNC_PS_GROUP);
					if (syncBit & NXP_WIFI_EVENT_BIT(WLAN_REASON_PS_ENTER)) {
						status = NXP_WIFI_RET_SUCCESS;
					} else {
						status = NXP_WIFI_RET_TIMEOUT;
					}
				}

				if (status != NXP_WIFI_RET_SUCCESS) {
					LOG_DBG("Wi-Fi power save is already enabled");
					return -EAGAIN;
				}

				k_event_clear(&s_nxp_wifi_SyncEvent, NXP_WIFI_SYNC_PS_GROUP);
				ret = wlan_ieeeps_on((unsigned int)WAKE_ON_UNICAST |
						     (unsigned int)WAKE_ON_MAC_EVENT |
						     (unsigned int)WAKE_ON_MULTICAST |
						     (unsigned int)WAKE_ON_ARP_BROADCAST);

				if (ret != WM_SUCCESS) {
					status = NXP_WIFI_RET_FAIL;
				}

				if (status == NXP_WIFI_RET_SUCCESS) {
					syncBit = k_event_wait(&s_nxp_wifi_SyncEvent,
							       NXP_WIFI_SYNC_PS_GROUP, false,
							       NXP_WIFI_SYNC_TIMEOUT_MS);
					k_event_clear(&s_nxp_wifi_SyncEvent,
						      NXP_WIFI_SYNC_PS_GROUP);
					if (syncBit & NXP_WIFI_EVENT_BIT(WLAN_REASON_PS_ENTER)) {
						status = NXP_WIFI_RET_SUCCESS;
					} else {
						status = NXP_WIFI_RET_TIMEOUT;
					}
				}

				if (status != NXP_WIFI_RET_SUCCESS) {
					LOG_DBG("Wi-Fi power save is already enabled");
					return -EAGAIN;
				}
			}
			break;
		case WIFI_PS_PARAM_LISTEN_INTERVAL:
			wlan_configure_listen_interval((int)params->listen_interval);
			break;
		case WIFI_PS_PARAM_WAKEUP_MODE:
			if (params->wakeup_mode == WIFI_PS_WAKEUP_MODE_DTIM) {
				wlan_configure_listen_interval(0);
			}
			break;
		case WIFI_PS_PARAM_MODE:
			if (params->mode == WIFI_PS_MODE_WMM) {
				ret = wlan_set_wmm_uapsd(1);

				if (ret != WM_SUCCESS) {
					status = NXP_WIFI_RET_FAIL;
				}
			} else if (params->mode == WIFI_PS_MODE_LEGACY) {
				ret = wlan_set_wmm_uapsd(0);

				if (ret != WM_SUCCESS) {
					status = NXP_WIFI_RET_FAIL;
				}
			}
			break;
		case WIFI_PS_PARAM_TIMEOUT:
			wlan_configure_delay_to_ps((int)params->timeout_ms);
			break;
		default:
			status = NXP_WIFI_RET_FAIL;
			break;
		}
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		LOG_ERR("Failed to set Wi-Fi power save");
		return -EAGAIN;
	}

	return 0;
}

int nxp_wifi_get_power_save(const struct device *dev, struct wifi_ps_config *config)
{
	nxp_wifi_ret_t status = NXP_WIFI_RET_SUCCESS;
	interface_t *if_handle = (interface_t *)dev->data;

	if (if_handle->state.interface != WLAN_BSS_TYPE_STA) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EIO;
	}

	if (s_nxp_wifi_State != NXP_WIFI_STARTED) {
		status = NXP_WIFI_RET_NOT_READY;
	}

	if (status == NXP_WIFI_RET_SUCCESS) {
		if (config) {
			if (wlan_is_power_save_enabled()) {
				config->ps_params.enabled = WIFI_PS_ENABLED;
			} else {
				config->ps_params.enabled = WIFI_PS_DISABLED;
			}

			config->ps_params.listen_interval = wlan_get_listen_interval();

			config->ps_params.timeout_ms = wlan_get_delay_to_ps();
			if (config->ps_params.listen_interval) {
				config->ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL;
			} else {
				config->ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM;
			}

			if (wlan_is_wmm_uapsd_enabled()) {
				config->ps_params.mode = WIFI_PS_MODE_WMM;
			} else {
				config->ps_params.mode = WIFI_PS_MODE_LEGACY;
			}
		} else {
			status = NXP_WIFI_RET_FAIL;
		}
	}

	if (status != NXP_WIFI_RET_SUCCESS) {
		LOG_ERR("Failed to get Wi-Fi power save config");
		return -EAGAIN;
	}

	return 0;
}

static int nxp_wifi_reg_domain(const struct device *dev, struct wifi_reg_domain *reg_domain)
{
	int ret;
	uint8_t index = 0;
	chan_freq_power_t *nxp_wifi_chan_freq_power = NULL;
	int nxp_wifi_cfp_no = 0;

	if(reg_domain->oper == WIFI_MGMT_GET)
	{
		nxp_wifi_chan_freq_power = (chan_freq_power_t *)wlan_get_regulatory_domain(0, &nxp_wifi_cfp_no);
		if(nxp_wifi_chan_freq_power != NULL)
		{
			for(int i = 0; i < nxp_wifi_cfp_no; i++)
			{
				reg_domain->chan_info[i].center_frequency = nxp_wifi_chan_freq_power[i].freq;
				reg_domain->chan_info[i].max_power = nxp_wifi_chan_freq_power[i].max_tx_power;
				reg_domain->chan_info[i].supported = 1;
				reg_domain->chan_info[i].passive_only = nxp_wifi_chan_freq_power[i].passive_scan_or_radar_detect;
				reg_domain->chan_info[i].dfs = nxp_wifi_chan_freq_power[i].passive_scan_or_radar_detect;
			}
			index = nxp_wifi_cfp_no;
		}

		nxp_wifi_chan_freq_power = (chan_freq_power_t *)wlan_get_regulatory_domain(1, &nxp_wifi_cfp_no);
		if(nxp_wifi_chan_freq_power != NULL)
		{
			for(int i = 0; i < nxp_wifi_cfp_no; i++)
			{
				reg_domain->chan_info[index + i].center_frequency = nxp_wifi_chan_freq_power[i].freq;
				reg_domain->chan_info[index + i].max_power = nxp_wifi_chan_freq_power[i].max_tx_power;
				reg_domain->chan_info[index + i].supported = 1;
				reg_domain->chan_info[index + i].passive_only = nxp_wifi_chan_freq_power[i].passive_scan_or_radar_detect;
				reg_domain->chan_info[index + i].dfs = nxp_wifi_chan_freq_power[i].passive_scan_or_radar_detect;
			}
			index += nxp_wifi_cfp_no;
		}
		reg_domain->num_channels = index;
		wifi_get_country_code(reg_domain->country_code);
	}
	else
	{
		if(is_uap_started())
		{
			LOG_ERR("region code can not be set after uAP start!");
			return -EAGAIN;
		}

		ret = wlan_set_country_code(reg_domain->country_code);
		if(ret != WM_SUCCESS)
		{
			LOG_ERR("Unable to set country code: %s", reg_domain->country_code);
			return -EAGAIN;
		}
	}
	return 0;
}

static int nxp_wifi_set_twt(const struct device *dev, struct wifi_twt_params *params)
{
    wlan_twt_setup_config_t twt_setup_conf;
    wlan_twt_teardown_config_t teardown_conf;
    int ret = -1;;

    if(WIFI_TWT_INDIVIDUAL == params->negotiation_type)
    {
        params->negotiation_type = 0;
    }
    else if(WIFI_TWT_BROADCAST == params->negotiation_type)
    {
        params->negotiation_type = 3;
    }

    if(WIFI_TWT_SETUP == params->operation)
    {
        twt_setup_conf.implicit = params->setup.implicit;
        twt_setup_conf.announced = params->setup.announce;
        twt_setup_conf.trigger_enabled = params->setup.trigger;
        twt_setup_conf.twt_info_disabled = params->setup.twt_info_disable;
        twt_setup_conf.negotiation_type = params->negotiation_type;
        twt_setup_conf.twt_wakeup_duration = params->setup.twt_wake_interval;
        twt_setup_conf.flow_identifier = params->flow_id;
        twt_setup_conf.hard_constraint = 1;
        twt_setup_conf.twt_exponent = params->setup.exponent;
        twt_setup_conf.twt_mantissa = params->setup.twt_interval;
        twt_setup_conf.twt_request = params->setup.responder;
        
        ret = wlan_set_twt_setup_cfg(&twt_setup_conf);
    }
    else if(WIFI_TWT_TEARDOWN == params->operation)
    {
        teardown_conf.flow_identifier = params->flow_id;
        teardown_conf.negotiation_type = params->negotiation_type;
        teardown_conf.teardown_all_twt = params->teardown.teardown_all;

        ret = wlan_set_twt_teardown_cfg(&teardown_conf);
    }

    return ret;
}

static int nxp_wifi_set_btwt(const struct device *dev, struct wifi_twt_params *params)
{
    wlan_btwt_config_t btwt_config;
    int ret = -1;;

    btwt_config.action = 1;
    btwt_config.sub_id = params->btwt.sub_id;
    btwt_config.nominal_wake = params->btwt.nominal_wake;
    btwt_config.max_sta_support = params->btwt.max_sta_support;
    btwt_config.twt_mantissa = params->btwt.twt_interval;
    btwt_config.twt_offset = params->btwt.twt_offset;
    btwt_config.twt_exponent = params->btwt.twt_exponent;
    btwt_config.sp_gap = params->btwt.sp_gap;

    ret = wlan_set_btwt_cfg(&btwt_config);

    return ret;
}

static int nxp_wifi_set_rts_threshold(const struct device *dev, unsigned int rts_threshold)
{
	int ret = -1;

#if CONFIG_WIFI_RTS_THRESHOLD
	if (rts_threshold == -1) {
		rts_threshold = MLAN_RTS_MAX_VALUE;
	}

	ret = wlan_set_rts(rts_threshold);
#endif

	return ret;
}

static int nxp_wifi_ap_set_rts_threshold(const struct device *dev, unsigned int rts_threshold)
{
	int ret = -1;

#if CONFIG_WIFI_RTS_THRESHOLD
	if (rts_threshold == -1) {
		rts_threshold = MLAN_RTS_MAX_VALUE;
	}

	ret = wlan_set_uap_rts(rts_threshold);
#endif

	return ret;
}

static void nxp_wifi_sta_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	interface_t *intf = dev->data;

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	intf->netif = iface;
#ifdef CONFIG_WIFI_NM
	wifi_nm_register_mgd_type_iface(wifi_nm_get_instance("wifi_supplicant"),
			WIFI_TYPE_STA, iface);
#endif
	g_mlan.state.interface = WLAN_BSS_TYPE_STA;

#ifndef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
	int ret;

	if (s_nxp_wifi_State == NXP_WIFI_NOT_INITIALIZED) {
		/* Initialize the wifi subsystem */
		ret = nxp_wifi_wlan_init();
		if (ret) {
			LOG_ERR("wlan initialization failed");
			return;
		}
		ret = nxp_wifi_wlan_start();
		if (ret) {
			LOG_ERR("wlan start failed");
			return;
		}
	}
#endif
}

#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT

static void nxp_wifi_uap_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	interface_t *intf = dev->data;
	int ret;

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	intf->netif = iface;
#ifdef CONFIG_WIFI_NM
	wifi_nm_register_mgd_type_iface(wifi_nm_get_instance("hostapd"),
			WIFI_TYPE_SAP, iface);
#endif
	g_uap.state.interface = WLAN_BSS_TYPE_UAP;

	if (s_nxp_wifi_State == NXP_WIFI_NOT_INITIALIZED) {
		/* Initialize the wifi subsystem */
		ret = nxp_wifi_wlan_init();
		if (ret) {
			LOG_ERR("wlan initialization failed");
			return;
		}
		ret = nxp_wifi_wlan_start();
		if (ret) {
			LOG_ERR("wlan start failed");
			return;
		}
	}
}

#endif

static NXP_WIFI_SET_FUNC_ATTR int nxp_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
#if defined(CONFIG_NET_STATISTICS_WIFI)
	interface_t *if_handle = (interface_t *)dev->data;
	const int pkt_len = net_pkt_get_len(pkt);
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
#endif

	/* Enqueue packet for transmission */
	if (nxp_wifi_internal_tx(dev, pkt) != WM_SUCCESS) {
		goto out;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	if_handle->stats.bytes.sent += pkt_len;
	if_handle->stats.pkts.tx++;

	if (net_eth_is_addr_multicast(&hdr->dst)) {
		if_handle->stats.multicast.tx++;
	} else if (net_eth_is_addr_broadcast(&hdr->dst)) {
		if_handle->stats.broadcast.tx++;
	}
#endif

	return 0;

out:
#if defined(CONFIG_NET_STATISTICS_WIFI)
	if_handle->stats.errors.tx++;
#endif
	return -EIO;
}

static NXP_WIFI_SET_FUNC_ATTR int nxp_wifi_recv(struct net_if *iface, struct net_pkt *pkt)
{
#if defined(CONFIG_NET_STATISTICS_WIFI)
	interface_t *if_handle = (interface_t *)(net_if_get_device(iface)->data);
	const int pkt_len = net_pkt_get_len(pkt);
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
#endif

	if (net_recv_data(iface, pkt) < 0) {
		goto out;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	if (net_eth_is_addr_broadcast(&hdr->dst)) {
		if_handle->stats.broadcast.rx++;
	} else if (net_eth_is_addr_multicast(&hdr->dst)) {
		if_handle->stats.multicast.rx++;
	}

	if_handle->stats.bytes.received += pkt_len;
	if_handle->stats.pkts.rx++;
#endif

	return 0;

out:
#if defined(CONFIG_NET_STATISTICS_WIFI)
	if_handle->stats.errors.rx++;
#endif
	return -EIO;
}

#ifdef CONFIG_RW610
extern void WL_MCI_WAKEUP0_DriverIRQHandler(void);
extern void WL_MCI_WAKEUP_DONE0_DriverIRQHandler(void);
#endif

static int nxp_wifi_dev_init(const struct device *dev)
{
	struct nxp_wifi_dev *nxp_wifi = &nxp_wifi0;

	k_mutex_init(&nxp_wifi->mutex);

	nxp_wifi_shell_register(nxp_wifi);

#ifdef CONFIG_RW610
	IRQ_CONNECT(IMU_IRQ_N, IMU_IRQ_P, WL_MCI_WAKEUP0_DriverIRQHandler, 0, 0);
	irq_enable(IMU_IRQ_N);
	IRQ_CONNECT(IMU_WAKEUP_IRQ_N, IMU_WAKEUP_IRQ_P, WL_MCI_WAKEUP_DONE0_DriverIRQHandler, 0, 0);
	irq_enable(IMU_WAKEUP_IRQ_N);
#if (DT_INST_PROP(0, wakeup_source))
	EnableDeepSleepIRQ(IMU_IRQ_N);
#endif
#endif

	return 0;
}

static int nxp_wifi_set_config(const struct device *dev, enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	interface_t *if_handle = (interface_t *)dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(if_handle->mac_address, config->mac_address.addr, 6);

		net_if_set_link_addr(if_handle->netif, if_handle->mac_address,
				     sizeof(if_handle->mac_address), NET_LINK_ETHERNET);

		if (if_handle->state.interface == WLAN_BSS_TYPE_STA) {
			if (wlan_set_sta_mac_addr(if_handle->mac_address)) {
				LOG_ERR("Failed to set Wi-Fi MAC Address");
				return -ENOEXEC;
			}
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
		} else if (if_handle->state.interface == WLAN_BSS_TYPE_UAP) {
			if (wlan_set_uap_mac_addr(if_handle->mac_address)) {
				LOG_ERR("Failed to set Wi-Fi MAC Address");
				return -ENOEXEC;
			}
#endif
		} else {
			LOG_ERR("Invalid Interface index");
			return -ENOEXEC;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_RW610)
void device_pm_dump_wakeup_source(void)
{
	if (POWER_GetWakeupStatus(IMU_IRQ_N)) {
		LOG_INF("Wakeup by WLAN");
		POWER_ClearWakeupStatus(IMU_IRQ_N);
	} else if (POWER_GetWakeupStatus(41)) {
		LOG_INF("Wakeup by OSTIMER");
		POWER_ClearWakeupStatus(41);
	} else if (POWER_GetWakeupStatus(32)) {
		LOG_INF("Wakeup by RTC");
		POWER_ClearWakeupStatus(32);
	}
}

static int device_wlan_pm_action(const struct device *dev, enum pm_device_action pm_action)
{
	int ret = 0;

	switch (pm_action) {
	case PM_DEVICE_ACTION_SUSPEND:
		if (!wlan_host_sleep_state || !wlan_is_started() || wakelock_isheld()
#ifdef CONFIG_NXP_WIFI_WMM_UAPSD
		    || wlan_is_wmm_uapsd_enabled()
#endif
		)
			return -EBUSY;
		/*
		 * Trigger host sleep handshake here. Before handshake is done, host is not allowed
		 * to enter low power mode
		 */
		if (!is_hs_handshake_done) {
			is_hs_handshake_done = WLAN_HOSTSLEEP_IN_PROCESS;
			ret = wlan_hs_send_event(HOST_SLEEP_HANDSHAKE, NULL);
			if (ret != 0) {
				return -EFAULT;
			}
			return -EBUSY;
		}
		if (is_hs_handshake_done == WLAN_HOSTSLEEP_IN_PROCESS) {
			return -EBUSY;
		}
		if (is_hs_handshake_done == WLAN_HOSTSLEEP_FAIL) {
			is_hs_handshake_done = 0;
			return -EFAULT;
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * Cancel host sleep in firmware and dump wakekup source.
		 * If sleep state is periodic, start timer to keep host in full power state for 5s.
		 * User can use this time to issue other commands.
		 */
		if (is_hs_handshake_done == WLAN_HOSTSLEEP_SUCCESS) {
			ret = wlan_hs_send_event(HOST_SLEEP_EXIT, NULL);
			if (ret != 0) {
				return -EFAULT;
			}
			device_pm_dump_wakeup_source();
			/* reset hs hanshake flag after waking up */
			is_hs_handshake_done = 0;
			if (wlan_host_sleep_state == HOST_SLEEP_ONESHOT) {
				wlan_host_sleep_state = HOST_SLEEP_DISABLE;
			}
		}
		break;
	default:
		break;
	}
	return ret;
}

PM_DEVICE_DT_INST_DEFINE(0, device_wlan_pm_action);
#endif

static const struct wifi_mgmt_ops nxp_wifi_sta_mgmt = {
	.get_version = nxp_wifi_version,
	.scan = nxp_wifi_scan,
	.connect = nxp_wifi_connect,
	.disconnect = nxp_wifi_disconnect,
	.reg_domain = nxp_wifi_reg_domain,
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
	.ap_enable = nxp_wifi_start_ap,
	.ap_bandwidth = nxp_wifi_ap_bandwidth,
	.ap_disable = nxp_wifi_stop_ap,
#endif
	.iface_status = nxp_wifi_status,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats = nxp_wifi_stats,
#endif
	.set_11k_enable = nxp_wifi_11k_enable,
	.set_power_save = nxp_wifi_power_save,
	.get_power_save_config = nxp_wifi_get_power_save,
	.set_twt = nxp_wifi_set_twt,
	.set_rts_threshold = nxp_wifi_set_rts_threshold,
};

#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT)
static const struct zep_wpa_supp_dev_ops nxp_wifi_drv_ops = {
	.init = wifi_nxp_wpa_supp_dev_init,
	.deinit                   = wifi_nxp_wpa_supp_dev_deinit,
	.scan2                    = wifi_nxp_wpa_supp_scan2,
	.scan_abort               = wifi_nxp_wpa_supp_scan_abort,
	.get_scan_results2        = wifi_nxp_wpa_supp_scan_results_get,
	.deauthenticate           = wifi_nxp_wpa_supp_deauthenticate,
	.authenticate             = wifi_nxp_wpa_supp_authenticate,
	.associate                = wifi_nxp_wpa_supp_associate,
	.set_key                  = wifi_nxp_wpa_supp_set_key,
	.set_supp_port            = wifi_nxp_wpa_supp_set_supp_port,
	.signal_poll              = wifi_nxp_wpa_supp_signal_poll,
	.send_mlme                = wifi_nxp_wpa_supp_send_mlme,
	.get_wiphy                = wifi_nxp_wpa_supp_get_wiphy,
	.get_capa                 = wifi_nxp_wpa_supp_get_capa,
	.get_conn_info            = wifi_nxp_wpa_supp_get_conn_info,
	.set_country              = wifi_nxp_wpa_supp_set_country,
	.get_country              = wifi_nxp_wpa_supp_get_country,
#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	.hapd_init                = wifi_nxp_hostapd_dev_init,
	.hapd_deinit              = wifi_nxp_hostapd_dev_deinit,
#endif
	.init_ap                  = wifi_nxp_wpa_supp_init_ap,
	.set_ap                   = wifi_nxp_hostapd_set_ap,
	.stop_ap                  = wifi_nxp_hostapd_stop_ap,
	.sta_remove               = wifi_nxp_hostapd_sta_remove,
	.sta_add                  = wifi_nxp_hostapd_sta_add,
	.do_acs                   = wifi_nxp_hostapd_do_acs,
#endif
	.dpp_listen               = wifi_nxp_wpa_dpp_listen,
	.remain_on_channel        = wifi_nxp_wpa_supp_remain_on_channel,
	.cancel_remain_on_channel = wifi_nxp_wpa_supp_cancel_remain_on_channel,
	.send_action_cancel_wait  = wifi_nxp_wpa_supp_cancel_action_wait,
};
#endif

static const struct net_wifi_mgmt_offload nxp_wifi_sta_apis = {
	.wifi_iface.iface_api.init = nxp_wifi_sta_init,
	.wifi_iface.set_config = nxp_wifi_set_config,
	.wifi_iface.send = nxp_wifi_send,
	.wifi_mgmt_api = &nxp_wifi_sta_mgmt,
#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT)
	.wifi_drv_ops = &nxp_wifi_drv_ops,
#endif
};

NET_DEVICE_INIT_INSTANCE(wifi_nxp_sta, "ml", 0, nxp_wifi_dev_init, PM_DEVICE_DT_INST_GET(0),
			 &g_mlan,
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
			 &wpa_supp_ops,
#else
			 NULL,
#endif
			 CONFIG_WIFI_INIT_PRIORITY, &nxp_wifi_sta_apis, ETHERNET_L2,
			 NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

#ifdef CONFIG_NXP_WIFI_SOFTAP_SUPPORT
static const struct wifi_mgmt_ops nxp_wifi_uap_mgmt = {
	.ap_enable = nxp_wifi_start_ap,
	.ap_disable = nxp_wifi_stop_ap,
	.iface_status = nxp_wifi_status,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats = nxp_wifi_stats,
#endif
	.set_power_save = nxp_wifi_power_save,
	.get_power_save_config = nxp_wifi_get_power_save,
	.set_btwt = nxp_wifi_set_btwt,
	.ap_bandwidth = nxp_wifi_ap_bandwidth,
	.ap_config_params = nxp_wifi_ap_config_params,
	.set_rts_threshold = nxp_wifi_ap_set_rts_threshold,
};

static const struct net_wifi_mgmt_offload nxp_wifi_uap_apis = {
	.wifi_iface.iface_api.init = nxp_wifi_uap_init,
	.wifi_iface.set_config = nxp_wifi_set_config,
	.wifi_iface.send = nxp_wifi_send,
	.wifi_mgmt_api = &nxp_wifi_uap_mgmt,
#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT)
	.wifi_drv_ops = &nxp_wifi_drv_ops,
#endif
};

NET_DEVICE_INIT_INSTANCE(wifi_nxp_uap, "ua", 1, NULL, NULL, &g_uap,
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT
			 &wpa_supp_ops,
#else
			 NULL,
#endif
			 CONFIG_WIFI_INIT_PRIORITY, &nxp_wifi_uap_apis, ETHERNET_L2,
			 NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
#endif
