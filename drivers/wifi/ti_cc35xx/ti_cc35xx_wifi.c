/**
 * Copyright (c) 2025 Conclusive Engineering sp. z o. o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <wlan_if.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#define TI_CC35XX_DOMAIN_LEN         3
#define TI_CC35XX_CONNECT_TIMEOUT_MS (MSEC_PER_SEC * 10)
#define TI_CC35XX_MAX_NUM_STA        4
#define TI_CC35XX_MAX_SCAN_RESULTS   30
#define TI_CC35XX_BAND_MASK          GENMASK(1, 0)

#define TI_CC35XX_SEC_TYPE_MASK                GENMASK(3, 0)
#define TI_CC35XX_SEC_TYPE_BITMAP_OPEN         (0)
#define TI_CC35XX_SEC_TYPE_BITMAP_WEP          BIT(0)
#define TI_CC35XX_SEC_TYPE_BITMAP_WPA          BIT(1)
#define TI_CC35XX_SEC_TYPE_BITMAP_WPA2         BIT(2)
#define TI_CC35XX_SEC_TYPE_BITMAP_WPA3         BIT(3)
#define TI_CC35XX_SEC_TYPE_BITMAP_PMF_CAPABLE  BIT(4)
#define TI_CC35XX_SEC_TYPE_BITMAP_PMF_REQUIRED BIT(5)

#define DT_DRV_COMPAT ti_cc35xx_wlan

LOG_MODULE_REGISTER(DT_DRV_COMPAT, CONFIG_WIFI_LOG_LEVEL);

enum ti_cc35xx_wifi_state {
	TI_CC35XX_INACTIVE,
	TI_CC35XX_STA_CONNECTING,
	TI_CC35XX_STA_CONNECTED,
	TI_CC35XX_AP_STARTED,
};

static struct ti_cc35xx_wifi_priv {
	struct net_if *iface;
	char mac_addr_sta[WIFI_MAC_ADDR_LEN];
	char mac_addr_ap[WIFI_MAC_ADDR_LEN];
	scan_result_cb_t scan_res_cb;
	uint8_t frame_buf[NET_ETH_MAX_FRAME_SIZE];
	struct k_mutex dom_lock;
	struct k_timer connect_timer;
	struct {
		enum ti_cc35xx_wifi_state state;
		char ssid[WIFI_SSID_MAX_LEN + 1];
		char bssid[WIFI_MAC_ADDR_LEN];
		enum wifi_security_type security;
		uint8_t domain[TI_CC35XX_DOMAIN_LEN];
	} status;
} ti_cc35xx_wifi_priv;

static void ti_cc35xx_wifi_iface_init(struct net_if *iface)
{
	struct ti_cc35xx_wifi_priv *priv = iface->if_dev->dev->data;
	struct ethernet_context *eth_ctx;
	WlanMacAddress_t mac = {};

	priv->iface = iface;

	mac.roleType = WLAN_ROLE_STA;
	Wlan_Get(WLAN_GET_MACADDRESS, &mac);
	memcpy(priv->mac_addr_sta, mac.pMacAddress, WIFI_MAC_ADDR_LEN);

	mac.roleType = WLAN_ROLE_AP;
	Wlan_Get(WLAN_GET_MACADDRESS, &mac);
	memcpy(priv->mac_addr_ap, mac.pMacAddress, WIFI_MAC_ADDR_LEN);

	net_if_set_link_addr(iface, priv->mac_addr_sta, WIFI_MAC_ADDR_LEN, NET_LINK_ETHERNET);

	net_if_dormant_on(iface);
	net_if_carrier_off(iface);
	eth_ctx = net_if_l2_data(iface);
	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	ethernet_init(iface);
}

static int ti_cc35xx_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	size_t len = net_pkt_get_len(pkt);
	WlanRole_e role;
	int ret;

	if (len > NET_ETH_MAX_FRAME_SIZE) {
		LOG_ERR("Packet size too large\n");
		ret = -ENOBUFS;
		goto out;
	}

	ret = net_pkt_read(pkt, priv->frame_buf, len);
	if (ret < 0) {
		goto out;
	}

	role = priv->status.state == TI_CC35XX_AP_STARTED ? WLAN_ROLE_AP : WLAN_ROLE_STA;
	ret = Wlan_EtherPacketSend(role, priv->frame_buf, len, 0);

out:
	net_pkt_unref(pkt);

	return ret;
}

static int ti_cc35xx_wifi_scan(const struct device *dev, struct wifi_scan_params *params,
			       scan_result_cb_t cb)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	scanCommon_t common = {
		.Band = BAND_SEL_BOTH,
	};
	int scan_count, ret;

	if (priv->status.state == TI_CC35XX_AP_STARTED) {
		LOG_INF("Scanning not supported in AP mode\n");
		return -ENOTSUP;
	}

	if (priv->scan_res_cb || priv->status.state == TI_CC35XX_STA_CONNECTING) {
		LOG_INF("Scan in progress\n");
		return -EINPROGRESS;
	}
	priv->scan_res_cb = cb;

	if (params->bands & ~TI_CC35XX_BAND_MASK) {
		return -ENOTSUP;
	}
	common.Band = params->bands ? (params->bands & TI_CC35XX_BAND_MASK) - 1 : BAND_SEL_BOTH;

	scan_count = params->max_bss_cnt ? params->max_bss_cnt : TI_CC35XX_MAX_SCAN_RESULTS;
	ret = Wlan_Scan(WLAN_ROLE_STA, &common, scan_count);

	return ret;
}

static int ti_cc35xx_wifi_connect(const struct device *dev, struct wifi_connect_req_params *params)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	int ret, type, key_len = 0;
	const char *key = NULL;
	k_timeout_t timeout;

	if (priv->status.state != TI_CC35XX_INACTIVE || priv->scan_res_cb) {
		return -EBUSY;
	}

	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		type = WLAN_SEC_TYPE_OPEN;
		break;
	case WIFI_SECURITY_TYPE_PSK:
	/* Fall-through. */
	case WIFI_SECURITY_TYPE_PSK_SHA256:
	/* Fall-through. */
	case WIFI_SECURITY_TYPE_WPA_PSK:
	/* Fall-through. */
	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		type = WLAN_SEC_TYPE_WPA_WPA2;
		key = params->psk;
		key_len = params->psk_length;
		break;
	case WIFI_SECURITY_TYPE_SAE:
	case WIFI_SECURITY_TYPE_SAE_H2E:
	case WIFI_SECURITY_TYPE_SAE_AUTO:
		type = WLAN_SEC_TYPE_WPA3;
		/*
		 * wifi shell has no standard way of passing a SAE password.
		 * If the sae_password field is empty, look in the psk field.
		 */
		if (params->sae_password) {
			key = params->sae_password;
			key_len = params->sae_password_length;
		} else {
			key = params->psk;
			key_len = params->psk_length;
		}
		break;
	default:
		LOG_ERR("Unsupported security type: %d\n", params->security);
		return -ENOTSUP;
	}

	priv->status.security = params->security;
	priv->status.state = TI_CC35XX_STA_CONNECTING;
	ret = Wlan_Connect(params->ssid, params->ssid_length, NULL, type, key, key_len, 0);
	if (ret) {
		priv->status.state = TI_CC35XX_INACTIVE;
		return ret;
	}

	/*
	 * There is no feedback from Wi-Fi HAL in case of a connection timeout.
	 * Instead, set up a timer with requested timeout value, or a default
	 * timeout value if not specified by caller.
	 */
	if (params->timeout >= 0) {
		timeout = params->timeout ? K_MSEC(params->timeout * MSEC_PER_SEC)
					  : K_MSEC(TI_CC35XX_CONNECT_TIMEOUT_MS);
		k_timer_start(&priv->connect_timer, timeout, K_NO_WAIT);
	}

	return 0;
}

static int ti_cc35xx_wifi_disconnect(const struct device *dev)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;

	if (priv->status.state != TI_CC35XX_STA_CONNECTING &&
	    priv->status.state != TI_CC35XX_STA_CONNECTED) {
		return -EINVAL;
	}

	return Wlan_Disconnect(WLAN_ROLE_STA, NULL);
}

static void ti_cc35xx_wifi_receive(WlanRole_e role_id, uint8_t *input, uint32_t len)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(wlan0));
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	struct net_pkt *pkt;
	int ret;

	ARG_UNUSED(role_id);

	pkt = net_pkt_rx_alloc_with_buffer(priv->iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to allocate RX pkt\n");
		return;
	}

	ret = net_pkt_write(pkt, input, len);
	if (ret < 0) {
		LOG_ERR("Failed to write into pkt: %d\n", ret);
		goto err;
	}

	ret = net_recv_data(priv->iface, pkt);
	if (ret < 0) {
		LOG_ERR("Failed to receive pkt: %d\n", ret);
		goto err;
	}

	return;

err:
	net_pkt_unref(pkt);
}

static void ti_cc35xx_wifi_get_domain(struct ti_cc35xx_wifi_priv *priv, uint8_t *domain)
{
	k_mutex_lock(&priv->dom_lock, K_FOREVER);
	memcpy(domain, priv->status.domain, TI_CC35XX_DOMAIN_LEN);
	k_mutex_unlock(&priv->dom_lock);
}

static int ti_cc35xx_wifi_ap_enable(const struct device *dev,
				    struct wifi_connect_req_params *params)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	RoleUpApCmd_t role_params = {};
	WlanCtrlBlk_t ctrl = {
		.TxSendPaceThresh = 1,
		.TransmitQOnTxComplete = 0,
		.TxSendPaceTimeoutMsec = 16,
	};
	int ret, key_len = 0;
	const char *key = NULL;

	if (priv->status.state != TI_CC35XX_INACTIVE || priv->scan_res_cb) {
		return -EBUSY;
	}

	ret = Wlan_Set(WLAN_SET_TX_CTRL, &ctrl);
	if (ret) {
		return ret;
	}
	k_sleep(K_SECONDS(2));

	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		role_params.secParams.Type = WLAN_SEC_TYPE_OPEN;
		key = NULL;
		key_len = 0;
		break;
	case WIFI_SECURITY_TYPE_WPA_PSK:
		role_params.secParams.Type = WLAN_SEC_TYPE_WPA;
		key = params->psk;
		key_len = params->psk_length;
		if (!key_len) {
			LOG_ERR("Must specify PSK for WPA security\n");
			return -ENOTSUP;
		}
		break;
	case WIFI_SECURITY_TYPE_PSK:
	/* Fall-through. */
	case WIFI_SECURITY_TYPE_PSK_SHA256:
	/* Fall-through. */
	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		role_params.secParams.Type = WLAN_SEC_TYPE_WPA_WPA2;
		key = params->psk;
		key_len = params->psk_length;
		if (!key_len) {
			LOG_ERR("Must specify PSK for WPA2 security\n");
			return -ENOTSUP;
		}
		break;
	case WIFI_SECURITY_TYPE_SAE:
	case WIFI_SECURITY_TYPE_SAE_H2E:
	case WIFI_SECURITY_TYPE_SAE_AUTO:
		role_params.secParams.Type = WLAN_SEC_TYPE_WPA3;
		if (params->sae_password) {
			key = params->sae_password;
			key_len = params->sae_password_length;
		} else {
			key = params->psk;
			key_len = params->psk_length;
		}
		if (!key_len) {
			LOG_ERR("Must specify passphrase for WPA3 SAE security\n");
			return -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Unsupported security type: %d\n", params->security);
		return -ENOTSUP;
	}

	role_params.secParams.KeyLen = key_len;
	if (key) {
		role_params.secParams.Key = k_calloc(1, key_len + 1);
		if (!role_params.secParams.Key) {
			LOG_ERR("Failed to allocate memory\n");
			ret = -ENOMEM;
			goto out;
		}

		memcpy(role_params.secParams.Key, key, key_len);
	}

	ti_cc35xx_wifi_get_domain(priv, role_params.countryDomain);

	role_params.sta_limit = MIN(CONFIG_WIFI_MGMT_AP_MAX_NUM_STA, TI_CC35XX_MAX_NUM_STA);
	role_params.hidden = 0; /* Unsupported in Zephyr. */
	role_params.tx_pow = 0;
	role_params.channel = params->channel;
	role_params.ssid = k_calloc(1, params->ssid_length + 1);
	if (!role_params.ssid) {
		LOG_ERR("Failed to allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}
	memcpy(role_params.ssid, params->ssid, params->ssid_length);

	ret = Wlan_RoleDown(WLAN_ROLE_STA, WLAN_WAIT_FOREVER);
	if (ret) {
		LOG_ERR("Failed to change role: %d\n", ret);
		goto out;
	}

	ret = Wlan_RoleUp(WLAN_ROLE_AP, &role_params, WLAN_WAIT_FOREVER);
	if (ret) {
		LOG_ERR("Failed to change role: %d\n", ret);
		goto out;
	}

	strncpy(priv->status.ssid, params->ssid, params->ssid_length);
	memcpy(priv->status.bssid, params->bssid, sizeof(priv->status.bssid));

	priv->status.security = params->security;
	Wlan_EtherPacketRecvRegisterCallback(WLAN_ROLE_AP, ti_cc35xx_wifi_receive);
	priv->status.state = TI_CC35XX_AP_STARTED;
	net_if_set_link_addr(priv->iface, priv->mac_addr_ap, WIFI_MAC_ADDR_LEN, NET_LINK_ETHERNET);

	net_if_dormant_off(priv->iface);
	net_if_carrier_on(priv->iface);
	wifi_mgmt_raise_ap_enable_result_event(priv->iface, 0);

out:
	k_free(role_params.ssid);
	k_free(role_params.secParams.Key);

	return ret;
}

static int ti_cc35xx_wifi_ap_disable(const struct device *dev)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	RoleUpApCmd_t role_params = {};
	int ret;

	if (priv->status.state != TI_CC35XX_AP_STARTED) {
		return -EINVAL;
	}

	ret = Wlan_RoleDown(WLAN_ROLE_AP, WLAN_WAIT_FOREVER);
	if (ret) {
		LOG_ERR("Failed to change role: %d\n", ret);
		return ret;
	}

	wifi_mgmt_raise_ap_disable_result_event(priv->iface, 0);
	net_if_dormant_on(priv->iface);
	net_if_carrier_off(priv->iface);
	net_if_set_link_addr(priv->iface, priv->mac_addr_sta, WIFI_MAC_ADDR_LEN, NET_LINK_ETHERNET);
	Wlan_EtherPacketRecvRegisterCallback(WLAN_ROLE_AP, NULL);
	memset(priv->status.ssid, 0, sizeof(priv->status.ssid));
	memset(priv->status.bssid, 0, sizeof(priv->status.bssid));

	ti_cc35xx_wifi_get_domain(priv, role_params.countryDomain);
	ret = Wlan_RoleUp(WLAN_ROLE_STA, &role_params, WLAN_WAIT_FOREVER);
	if (ret) {
		LOG_ERR("Failed to change role: %d\n", ret);
		return ret;
	}
	priv->status.state = TI_CC35XX_INACTIVE;

	return 0;
}

static int ti_cc35xx_wifi_status(const struct device *dev, struct wifi_iface_status *status)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	WlanBeaconRssi_t rssi;
	WlanRole_current_channel_number chan = {};

	memset(status, 0, sizeof(*status));

	switch (priv->status.state) {
	case TI_CC35XX_INACTIVE:
		status->state = WIFI_STATE_INACTIVE;
		break;
	case TI_CC35XX_STA_CONNECTING:
		status->state = WIFI_STATE_SCANNING;
		break;
	case TI_CC35XX_STA_CONNECTED:
	/* Fall-through. */
	case TI_CC35XX_AP_STARTED:
		status->state = WIFI_STATE_COMPLETED;
		break;
	}

	if (priv->scan_res_cb) {
		status->state = WIFI_STATE_SCANNING;
	}

	status->security = priv->status.security;
	strncpy(status->ssid, priv->status.ssid, WIFI_SSID_MAX_LEN);
	status->ssid_len = strnlen(priv->status.ssid, WIFI_SSID_MAX_LEN);
	memcpy(status->bssid, priv->status.bssid, sizeof(priv->status.bssid));

	if (priv->status.state == TI_CC35XX_AP_STARTED) {
		status->iface_mode = WIFI_MODE_AP;
		chan.roleType = WLAN_ROLE_AP;
	} else {
		status->iface_mode = WIFI_MODE_INFRA;
		chan.roleType = WLAN_ROLE_STA;
	}

	Wlan_Get(WLAN_GET_ROLE_CHANNEL_NUMBER, &chan);
	status->channel = chan.channelNum;

	status->band = chan.channelNum < 32 ? WIFI_FREQ_BAND_2_4_GHZ : WIFI_FREQ_BAND_5_GHZ;

	Wlan_Get(WLAN_GET_RSSI, &rssi);
	status->rssi = rssi.rssi_data;

	return 0;
}

static int ti_cc35xx_wifi_reg_domain(const struct device *dev, struct wifi_reg_domain *reg_domain)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	int ret = 0;

	k_mutex_lock(&priv->dom_lock, K_FOREVER);
	if (reg_domain->oper == WIFI_MGMT_GET) {
		memcpy(reg_domain->country_code, priv->status.domain, WIFI_COUNTRY_CODE_LEN);
	} else {
		/* Can't update reg domain while AP is running. */
		if (priv->status.state == TI_CC35XX_AP_STARTED) {
			ret = -EBUSY;
			goto out;
		}

		memcpy(priv->status.domain, reg_domain->country_code, WIFI_COUNTRY_CODE_LEN);
		priv->status.domain[2] = 'I'; /* Indoor only. */
	}

out:
	k_mutex_unlock(&priv->dom_lock);

	return ret;
}

static struct wifi_mgmt_ops ti_cc35xx_wifi_mgmt_ops = {
	.scan = ti_cc35xx_wifi_scan,
	.connect = ti_cc35xx_wifi_connect,
	.disconnect = ti_cc35xx_wifi_disconnect,
	.ap_enable = ti_cc35xx_wifi_ap_enable,
	.ap_disable = ti_cc35xx_wifi_ap_disable,
	.iface_status = ti_cc35xx_wifi_status,
	.reg_domain = ti_cc35xx_wifi_reg_domain,
};

static const struct net_wifi_mgmt_offload ti_cc35xx_wifi_mgmt_offload_ops = {
	.wifi_iface.iface_api.init = ti_cc35xx_wifi_iface_init,
	.wifi_iface.send = ti_cc35xx_wifi_send,
	.wifi_mgmt_api = &ti_cc35xx_wifi_mgmt_ops,
};

static void ti_cc35xx_wifi_scan_results(struct ti_cc35xx_wifi_priv *priv, WlanEvent_t *ev)
{
	struct wifi_scan_result tmp;
	WlanNetworkEntry_t *entry;
	int sec_info, len, i;

	len = ev->Data.ScanResult.NetworkListResultLen;
	for (i = 0; i < len; i++) {
		entry = &ev->Data.ScanResult.NetworkListResult[i];

		memset(&tmp, 0, sizeof(tmp));
		memcpy(tmp.ssid, entry->Ssid, entry->SsidLen);
		tmp.ssid_length = entry->SsidLen;
		tmp.channel = entry->Channel;
		memcpy(tmp.mac, entry->Bssid, WIFI_MAC_ADDR_LEN);
		tmp.mac_length = WIFI_MAC_ADDR_LEN;
		tmp.rssi = entry->Rssi;
		tmp.band = tmp.channel < 32 ? WIFI_FREQ_BAND_2_4_GHZ : WIFI_FREQ_BAND_5_GHZ;

		sec_info = WLAN_SCAN_RESULT_SEC_TYPE_BITMAP(entry->SecurityInfo);
		switch (sec_info & TI_CC35XX_SEC_TYPE_MASK) {
		case TI_CC35XX_SEC_TYPE_BITMAP_OPEN:
			tmp.security = WIFI_SECURITY_TYPE_NONE;
			break;
		case TI_CC35XX_SEC_TYPE_BITMAP_WEP:
			tmp.security = WIFI_SECURITY_TYPE_WEP;
			break;
		case TI_CC35XX_SEC_TYPE_BITMAP_WPA:
			tmp.security = WIFI_SECURITY_TYPE_WPA_PSK;
			break;
		case TI_CC35XX_SEC_TYPE_BITMAP_WPA | TI_CC35XX_SEC_TYPE_BITMAP_WPA2:
		/* Fall-through */
		case TI_CC35XX_SEC_TYPE_BITMAP_WPA2:
			tmp.security = WIFI_SECURITY_TYPE_PSK;
			break;
		case TI_CC35XX_SEC_TYPE_BITMAP_WPA3:
			tmp.security = WIFI_SECURITY_TYPE_SAE;
			break;
		default:
			tmp.security = WIFI_SECURITY_TYPE_UNKNOWN;
		}

		if (sec_info & TI_CC35XX_SEC_TYPE_BITMAP_PMF_REQUIRED) {
			tmp.mfp = WIFI_MFP_REQUIRED;
		} else if (sec_info & TI_CC35XX_SEC_TYPE_BITMAP_PMF_CAPABLE) {
			tmp.mfp = WIFI_MFP_OPTIONAL;
		}

		priv->scan_res_cb(priv->iface, 0, &tmp);
	}

	/* End of scan event. */
	priv->scan_res_cb(priv->iface, 0, NULL);
	priv->scan_res_cb = NULL;
}

static void ti_cc35xx_wifi_update_peer(struct ti_cc35xx_wifi_priv *priv, WlanEvent_t *ev)
{
	struct wifi_ap_sta_info info = {
		.link_mode = WIFI_4,
		.mac_length = WIFI_MAC_ADDR_LEN,
	};

	if (ev->Id == WLAN_EVENT_ADD_PEER) {
		memcpy(info.mac, ev->Data.AddPeer.Mac, WIFI_MAC_ADDR_LEN);
		wifi_mgmt_raise_ap_sta_connected_event(priv->iface, &info);
	} else {
		memcpy(info.mac, ev->Data.RemovePeer.Mac, WIFI_MAC_ADDR_LEN);
		wifi_mgmt_raise_ap_sta_disconnected_event(priv->iface, &info);
	}
}

static void ti_cc35xx_wifi_event_handler(WlanEvent_t *event)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(wlan0));
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	struct net_if *iface = priv->iface;
	int reason;

	switch (event->Id) {
	case WLAN_EVENT_CONNECT:
		reason = WIFI_STATUS_CONN_SUCCESS;
		wifi_mgmt_raise_connect_result_event(iface, reason);
		net_if_dormant_off(iface);
		net_if_carrier_on(iface);
		strncpy(priv->status.ssid, event->Data.Connect.SsidName,
			event->Data.Connect.SsidLen);
		strncpy(priv->status.bssid, event->Data.Connect.Bssid, sizeof(priv->status.bssid));

		Wlan_EtherPacketRecvRegisterCallback(WLAN_ROLE_STA, ti_cc35xx_wifi_receive);
		priv->status.state = TI_CC35XX_STA_CONNECTED;
		break;
	case WLAN_EVENT_DISCONNECT:
		net_if_dormant_on(iface);
		net_if_carrier_off(iface);
		memset(priv->status.ssid, 0, sizeof(priv->status.ssid));
		memset(priv->status.bssid, 0, sizeof(priv->status.bssid));

		if (priv->status.state == TI_CC35XX_STA_CONNECTING) {
			reason = WIFI_STATUS_CONN_FAIL;
			wifi_mgmt_raise_connect_result_event(iface, reason);
		} else {
			reason = WIFI_REASON_DISCONN_SUCCESS;
			wifi_mgmt_raise_disconnect_result_event(iface, reason);
		}

		priv->status.state = TI_CC35XX_INACTIVE;
		Wlan_EtherPacketRecvRegisterCallback(WLAN_ROLE_STA, NULL);
		break;
	case WLAN_EVENT_SCAN_RESULT:
		ti_cc35xx_wifi_scan_results(priv, event);
		break;
	case WLAN_EVENT_ADD_PEER:
	/* Fall-through. */
	case WLAN_EVENT_REMOVE_PEER:
		ti_cc35xx_wifi_update_peer(priv, event);
		break;
	case WLAN_EVENT_CONNECTING:
		k_timer_stop(&priv->connect_timer);
		break;
	case WLAN_EVENT_ASSOCIATED:
		/* Nothing to be done. */
		break;
	case WLAN_EVENT_BLE_ENABLED:
		/* Nothing to be done. */
		break;
	case WLAN_EVENT_AUTHENTICATION_REJECTED:
		LOG_WRN("Authentication rejected (status %u)", event->Data.AuthStatusCode);
		break;
	case WLAN_EVENT_ASSOCIATION_REJECTED:
		LOG_WRN("Association rejected (status %u)", event->Data.AssocStatusCode);
		break;
	case WLAN_EVENT_GENERAL_ERROR:
		/* Scan timeout (no results found) — signal end of scan */
		if (priv->scan_res_cb) {
			priv->scan_res_cb(priv->iface, 0, NULL);
			priv->scan_res_cb = NULL;
		}
		break;
	case WLAN_EVENT_ERROR:
		LOG_ERR("ERROR module=%d err=%d sev=%d\n", event->Data.error.module,
			event->Data.error.error_num, event->Data.error.severity);
		break;

	default:
		LOG_ERR("Unhandled event: %d\n", event->Id);
		break;
	}
}

static void ti_cc35xx_wifi_connect_timeout(struct k_timer *timer)
{
	struct ti_cc35xx_wifi_priv *priv =
		CONTAINER_OF(timer, struct ti_cc35xx_wifi_priv, connect_timer);

	if (priv->status.state != TI_CC35XX_STA_CONNECTING) {
		return;
	}

	wifi_mgmt_raise_connect_result_event(priv->iface, WIFI_STATUS_CONN_TIMEOUT);
	Wlan_Disconnect(WLAN_ROLE_STA, NULL);
}

static int ti_cc35xx_wifi_init(const struct device *dev)
{
	struct ti_cc35xx_wifi_priv *priv = dev->data;
	WlanFWVersions_t fw_version = {};
	WlanCtrlBlk_t ctrl = {
		.TxSendPaceThresh = 1,
		.TransmitQOnTxComplete = 0,
		.TxSendPaceTimeoutMsec = 16,
	};
	RoleUpStaCmd_t role_params = {};
	uint8_t band = BAND_SEL_BOTH;
	uint32_t pwr_mode;
	int ret;

	ret = Wlan_Start(ti_cc35xx_wifi_event_handler);
	if (ret) {
		return ret;
	}

	k_sleep(K_SECONDS(1));

	Wlan_Get(WLAN_GET_FWVERSION, &fw_version);
	LOG_INF("FW %d.%d, api: %d, build: %d\n", fw_version.major_version,
		fw_version.minor_version, fw_version.api_version, fw_version.build_version);

	pwr_mode = POWER_MANAGEMENT_ALWAYS_ACTIVE_MODE;
	ret = Wlan_Set(WLAN_SET_POWER_MANAGEMENT, &pwr_mode);
	if (ret) {
		return ret;
	}

	ret = Wlan_Set(WLAN_SET_TX_CTRL, &ctrl);
	if (ret) {
		return ret;
	}

	ret = Wlan_Set(WLAN_SET_STA_WIFI_BAND, &band);
	if (ret) {
		return ret;
	}

	k_sleep(K_SECONDS(2));

	k_mutex_init(&priv->dom_lock);
	memcpy(priv->status.domain, "00I", 3);
	ti_cc35xx_wifi_get_domain(priv, role_params.countryDomain);
	ret = Wlan_RoleUp(WLAN_ROLE_STA, &role_params, WLAN_WAIT_FOREVER);
	if (ret) {
		return ret;
	}

	priv->status.state = TI_CC35XX_INACTIVE;

	k_timer_init(&priv->connect_timer, ti_cc35xx_wifi_connect_timeout, NULL);

	return 0;
}

ETH_NET_DEVICE_DT_INST_DEFINE(0, ti_cc35xx_wifi_init, NULL, &ti_cc35xx_wifi_priv, NULL,
			      CONFIG_WIFI_INIT_PRIORITY, &ti_cc35xx_wifi_mgmt_offload_ops,
			      CONFIG_WIFI_TI_CC35XX_MTU);

#ifdef CONFIG_NET_CONNECTION_MANAGER_CONNECTIVITY_WIFI_MGMT
CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));
#endif
