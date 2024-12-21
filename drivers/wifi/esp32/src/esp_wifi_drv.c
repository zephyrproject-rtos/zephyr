/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_wifi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_wifi, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
#include <zephyr/net/wifi_nm.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>
#endif
#include <zephyr/device.h>
#include <soc.h>
#include "esp_private/wifi.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_wpa.h"
#include <esp_mac.h>
#include "wifi/wifi_event.h"

#if CONFIG_SOC_SERIES_ESP32S2 || CONFIG_SOC_SERIES_ESP32C3
#include <esp_private/adc_share_hw_ctrl.h>
#endif /* CONFIG_SOC_SERIES_ESP32S2 || CONFIG_SOC_SERIES_ESP32C3 */

#define DHCPV4_MASK (NET_EVENT_IPV4_DHCP_BOUND | NET_EVENT_IPV4_DHCP_STOP)

/* use global iface pointer to support any ethernet driver */
/* necessary for wifi callback functions */
static struct net_if *esp32_wifi_iface;
static struct esp32_wifi_runtime esp32_data;

#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
static struct net_if *esp32_wifi_iface_ap;
static struct esp32_wifi_runtime esp32_ap_sta_data;
#endif

enum esp32_state_flag {
	ESP32_STA_STOPPED,
	ESP32_STA_STARTED,
	ESP32_STA_CONNECTING,
	ESP32_STA_CONNECTED,
	ESP32_AP_STARTED,
	ESP32_AP_CONNECTED,
	ESP32_AP_DISCONNECTED,
	ESP32_AP_STOPPED,
};

struct esp32_wifi_status {
	char ssid[WIFI_SSID_MAX_LEN + 1];
	char pass[WIFI_PSK_MAX_LEN + 1];
	wifi_auth_mode_t security;
	bool connected;
	uint8_t channel;
	int rssi;
};

struct esp32_wifi_runtime {
	uint8_t mac_addr[6];
	uint8_t frame_buf[NET_ETH_MAX_FRAME_SIZE];
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_stats_wifi stats;
#endif
	struct esp32_wifi_status status;
	scan_result_cb_t scan_cb;
	uint8_t state;
	uint8_t ap_connection_cnt;
};

static struct net_mgmt_event_callback esp32_dhcp_cb;

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			       struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		wifi_mgmt_raise_connect_result_event(iface, 0);
		break;
	default:
		break;
	}
}

static int esp32_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	struct esp32_wifi_runtime *data = dev->data;
	const int pkt_len = net_pkt_get_len(pkt);
	esp_interface_t ifx = data->state == ESP32_AP_CONNECTED ? ESP_IF_WIFI_AP : ESP_IF_WIFI_STA;

	/* Read the packet payload */
	if (net_pkt_read(pkt, data->frame_buf, pkt_len) < 0) {
		goto out;
	}

	/* Enqueue packet for transmission */
	if (esp_wifi_internal_tx(ifx, (void *)data->frame_buf, pkt_len) != ESP_OK) {
		goto out;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	data->stats.bytes.sent += pkt_len;
	data->stats.pkts.tx++;
#endif

	LOG_DBG("pkt sent %p len %d", pkt, pkt_len);
	return 0;

out:

	LOG_ERR("Failed to send packet");
#if defined(CONFIG_NET_STATISTICS_WIFI)
	data->stats.errors.tx++;
#endif
	return -EIO;
}

static esp_err_t eth_esp32_rx(void *buffer, uint16_t len, void *eb)
{
	struct net_pkt *pkt;

	if (esp32_wifi_iface == NULL) {
		esp_wifi_internal_free_rx_buffer(eb);
		LOG_ERR("network interface unavailable");
		return -EIO;
	}

	pkt = net_pkt_rx_alloc_with_buffer(esp32_wifi_iface, len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to allocate net buffer");
		esp_wifi_internal_free_rx_buffer(eb);
		return -EIO;
	}

	if (net_pkt_write(pkt, buffer, len) < 0) {
		LOG_ERR("Failed to write to net buffer");
		goto pkt_unref;
	}

	if (net_recv_data(esp32_wifi_iface, pkt) < 0) {
		LOG_ERR("Failed to push received data");
		goto pkt_unref;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	esp32_data.stats.bytes.received += len;
	esp32_data.stats.pkts.rx++;
#endif

	esp_wifi_internal_free_rx_buffer(eb);
	return 0;

pkt_unref:
	esp_wifi_internal_free_rx_buffer(eb);
	net_pkt_unref(pkt);

#if defined(CONFIG_NET_STATISTICS_WIFI)
	esp32_data.stats.errors.rx++;
#endif

	return -EIO;
}

#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
static esp_err_t wifi_esp32_ap_iface_rx(void *buffer, uint16_t len, void *eb)
{
	struct net_pkt *pkt;

	if (esp32_wifi_iface_ap == NULL) {
		esp_wifi_internal_free_rx_buffer(eb);
		LOG_ERR("network interface unavailable");
		return -EIO;
	}

	pkt = net_pkt_rx_alloc_with_buffer(esp32_wifi_iface_ap, len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		esp_wifi_internal_free_rx_buffer(eb);
		LOG_ERR("Failed to get net buffer");
		return -EIO;
	}

	if (net_pkt_write(pkt, buffer, len) < 0) {
		LOG_ERR("Failed to write pkt");
		goto pkt_unref;
	}

	if (net_recv_data(esp32_wifi_iface_ap, pkt) < 0) {
		LOG_ERR("Failed to push received data");
		goto pkt_unref;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	esp32_ap_sta_data.stats.bytes.received += len;
	esp32_ap_sta_data.stats.pkts.rx++;
#endif

	esp_wifi_internal_free_rx_buffer(eb);
	return 0;

pkt_unref:
	esp_wifi_internal_free_rx_buffer(eb);
	net_pkt_unref(pkt);

#if defined(CONFIG_NET_STATISTICS_WIFI)
	esp32_ap_sta_data.stats.errors.rx++;
#endif
	return -EIO;
}
#endif

static void scan_done_handler(void)
{
	uint16_t aps = 0;
	wifi_ap_record_t *ap_list_buffer;
	struct wifi_scan_result res = { 0 };

	esp_wifi_scan_get_ap_num(&aps);
	if (!aps) {
		LOG_INF("No Wi-Fi AP found");
		goto out;
	}

	ap_list_buffer = k_malloc(aps * sizeof(wifi_ap_record_t));
	if (ap_list_buffer == NULL) {
		LOG_INF("Failed to malloc buffer to print scan results");
		goto out;
	}

	if (esp_wifi_scan_get_ap_records(&aps, (wifi_ap_record_t *)ap_list_buffer) == ESP_OK) {
		for (int k = 0; k < aps; k++) {
			memset(&res, 0, sizeof(struct wifi_scan_result));
			int ssid_len = strnlen(ap_list_buffer[k].ssid, WIFI_SSID_MAX_LEN);

			res.ssid_length = ssid_len;
			strncpy(res.ssid, ap_list_buffer[k].ssid, ssid_len);
			res.rssi = ap_list_buffer[k].rssi;
			res.channel = ap_list_buffer[k].primary;

			memcpy(res.mac, ap_list_buffer[k].bssid, WIFI_MAC_ADDR_LEN);
			res.mac_length = WIFI_MAC_ADDR_LEN;

			switch (ap_list_buffer[k].authmode) {
			case WIFI_AUTH_OPEN:
				res.security = WIFI_SECURITY_TYPE_NONE;
				break;
			case WIFI_AUTH_WPA2_PSK:
				res.security = WIFI_SECURITY_TYPE_PSK;
				break;
			case WIFI_AUTH_WPA3_PSK:
				res.security = WIFI_SECURITY_TYPE_SAE;
				break;
			case WIFI_AUTH_WAPI_PSK:
				res.security = WIFI_SECURITY_TYPE_WAPI;
				break;
			case WIFI_AUTH_WPA2_ENTERPRISE:
				res.security = WIFI_SECURITY_TYPE_EAP;
				break;
			case WIFI_AUTH_WEP:
				res.security = WIFI_SECURITY_TYPE_WEP;
				break;
			case WIFI_AUTH_WPA_PSK:
				res.security = WIFI_SECURITY_TYPE_WPA_PSK;
				break;
			default:
				res.security = WIFI_SECURITY_TYPE_UNKNOWN;
				break;
			}

			if (esp32_data.scan_cb) {
				esp32_data.scan_cb(esp32_wifi_iface, 0, &res);

				/* ensure notifications get delivered */
				k_yield();
			}
		}
	} else {
		LOG_INF("Unable to retrieve AP records");
	}

	k_free(ap_list_buffer);

out:
	/* report end of scan event */
	esp32_data.scan_cb(esp32_wifi_iface, 0, NULL);
	esp32_data.scan_cb = NULL;
}

static void esp_wifi_handle_sta_connect_event(void *event_data)
{
	ARG_UNUSED(event_data);
	esp32_data.state = ESP32_STA_CONNECTED;
#if defined(CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4)
	net_dhcpv4_start(esp32_wifi_iface);
#else
	wifi_mgmt_raise_connect_result_event(esp32_wifi_iface, 0);
#endif
}

static void esp_wifi_handle_sta_disconnect_event(void *event_data)
{
	wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;

	if (esp32_data.state == ESP32_STA_CONNECTED) {
#if defined(CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4)
		net_dhcpv4_stop(esp32_wifi_iface);
#endif
		wifi_mgmt_raise_disconnect_result_event(esp32_wifi_iface, 0);
	} else {
		wifi_mgmt_raise_disconnect_result_event(esp32_wifi_iface, -1);
	}

	LOG_DBG("Disconnect reason: %d", event->reason);
	switch (event->reason) {
	case WIFI_REASON_AUTH_EXPIRE:
	case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
	case WIFI_REASON_AUTH_FAIL:
	case WIFI_REASON_HANDSHAKE_TIMEOUT:
	case WIFI_REASON_MIC_FAILURE:
		LOG_DBG("STA Auth Error");
		break;
	case WIFI_REASON_NO_AP_FOUND:
		LOG_DBG("AP Not found");
		break;
	default:
		break;
	}

	if (IS_ENABLED(CONFIG_ESP32_WIFI_STA_RECONNECT) &&
	    (event->reason != WIFI_REASON_ASSOC_LEAVE)) {
		esp32_data.state = ESP32_STA_CONNECTING;
		esp_wifi_connect();
	} else {
		esp32_data.state = ESP32_STA_STARTED;
	}
}

static void esp_wifi_handle_ap_connect_event(void *event_data)
{
#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
	struct net_if *iface = esp32_wifi_iface_ap;
	wifi_rxcb_t esp32_rx = wifi_esp32_ap_iface_rx;
#else
	struct net_if *iface = esp32_wifi_iface;
	wifi_rxcb_t esp32_rx = eth_esp32_rx;
#endif
	wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;

	LOG_DBG("Station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);

	struct wifi_ap_sta_info sta_info;

	sta_info.mac_length = WIFI_MAC_ADDR_LEN;
	memcpy(sta_info.mac, event->mac, WIFI_MAC_ADDR_LEN);
	wifi_mgmt_raise_ap_sta_connected_event(iface, &sta_info);

	if (!(esp32_data.ap_connection_cnt++)) {
		esp_wifi_internal_reg_rxcb(WIFI_IF_AP, esp32_rx);
	}
}

static void esp_wifi_handle_ap_disconnect_event(void *event_data)
{
#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
	struct net_if *iface = esp32_wifi_iface_ap;
#else
	struct net_if *iface = esp32_wifi_iface;
#endif
	wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;

	LOG_DBG("station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
	struct wifi_ap_sta_info sta_info;

	sta_info.mac_length = WIFI_MAC_ADDR_LEN;
	memcpy(sta_info.mac, event->mac, WIFI_MAC_ADDR_LEN);
	wifi_mgmt_raise_ap_sta_disconnected_event(iface, &sta_info);

	if (!(--esp32_data.ap_connection_cnt)) {
		esp_wifi_internal_reg_rxcb(WIFI_IF_AP, NULL);
	}
}

void esp_wifi_event_handler(const char *event_base, int32_t event_id, void *event_data,
			    size_t event_data_size, uint32_t ticks_to_wait)
{
#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
	struct net_if *iface_ap = esp32_wifi_iface_ap;
	struct esp32_wifi_runtime *ap_data = &esp32_ap_sta_data;
#else
	struct net_if *iface_ap = esp32_wifi_iface;
	struct esp32_wifi_runtime *ap_data = &esp32_data;
#endif

	LOG_DBG("Wi-Fi event: %d", event_id);
	switch (event_id) {
	case WIFI_EVENT_STA_START:
		esp32_data.state = ESP32_STA_STARTED;
		net_eth_carrier_on(esp32_wifi_iface);
		break;
	case WIFI_EVENT_STA_STOP:
		esp32_data.state = ESP32_STA_STOPPED;
		net_eth_carrier_off(esp32_wifi_iface);
		break;
	case WIFI_EVENT_STA_CONNECTED:
		esp_wifi_handle_sta_connect_event(event_data);
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		esp_wifi_handle_sta_disconnect_event(event_data);
		break;
	case WIFI_EVENT_SCAN_DONE:
		scan_done_handler();
		break;
	case WIFI_EVENT_AP_START:
		ap_data->state = ESP32_AP_STARTED;
		net_eth_carrier_on(iface_ap);
		wifi_mgmt_raise_ap_enable_result_event(iface_ap, 0);
		break;
	case WIFI_EVENT_AP_STOP:
		ap_data->state = ESP32_AP_STOPPED;
		net_eth_carrier_off(iface_ap);
		wifi_mgmt_raise_ap_disable_result_event(iface_ap, 0);
		break;
	case WIFI_EVENT_AP_STACONNECTED:
		ap_data->state = ESP32_AP_CONNECTED;
		esp_wifi_handle_ap_connect_event(event_data);
		break;
	case WIFI_EVENT_AP_STADISCONNECTED:
		ap_data->state = ESP32_AP_DISCONNECTED;
		esp_wifi_handle_ap_disconnect_event(event_data);
		break;
	default:
		break;
	}
}

static int esp32_wifi_disconnect(const struct device *dev)
{
	int ret = esp_wifi_disconnect();

	if (ret != ESP_OK) {
		LOG_INF("Failed to disconnect from hotspot");
		return -EAGAIN;
	}

	return 0;
}

static int esp32_wifi_connect(const struct device *dev,
			    struct wifi_connect_req_params *params)
{
	struct esp32_wifi_runtime *data = dev->data;
	struct net_if *iface = net_if_lookup_by_dev(dev);
	wifi_mode_t mode;
	int ret;

	if (data->state == ESP32_STA_CONNECTING || data->state == ESP32_STA_CONNECTED) {
		wifi_mgmt_raise_connect_result_event(iface, -1);
		return -EALREADY;
	}

	ret = esp_wifi_get_mode(&mode);
	if (ret) {
		LOG_ERR("Failed to get Wi-Fi mode (%d)", ret);
		return -EAGAIN;
	}

	if (IS_ENABLED(CONFIG_ESP32_WIFI_AP_STA_MODE) &&
	    (mode == ESP32_WIFI_MODE_AP || mode == ESP32_WIFI_MODE_APSTA)) {
		ret = esp_wifi_set_mode(ESP32_WIFI_MODE_APSTA);
	} else {
		ret = esp_wifi_set_mode(ESP32_WIFI_MODE_STA);
	}

	if (ret) {
		LOG_ERR("Failed to set Wi-Fi mode (%d)", ret);
		return -EAGAIN;
	}
	ret = esp_wifi_start();
	if (ret) {
		LOG_ERR("Failed to start Wi-Fi driver (%d)", ret);
		return -EAGAIN;
	}

	if (data->state != ESP32_STA_STARTED) {
		LOG_ERR("Wi-Fi not in station mode");
		wifi_mgmt_raise_connect_result_event(iface, -1);
		return -EIO;
	}

	data->state = ESP32_STA_CONNECTING;

	memcpy(data->status.ssid, params->ssid, params->ssid_length);
	data->status.ssid[params->ssid_length] = '\0';

	wifi_config_t wifi_config;

	memset(&wifi_config, 0, sizeof(wifi_config_t));

	memcpy(wifi_config.sta.ssid, params->ssid, params->ssid_length);
	wifi_config.sta.ssid[params->ssid_length] = '\0';
	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
		data->status.security = WIFI_AUTH_OPEN;
		wifi_config.sta.pmf_cfg.required = false;
		break;
	case WIFI_SECURITY_TYPE_PSK:
		memcpy(wifi_config.sta.password, params->psk, params->psk_length);
		wifi_config.sta.password[params->psk_length] = '\0';
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
		wifi_config.sta.pmf_cfg.required = false;
		data->status.security = WIFI_AUTH_WPA2_PSK;
		break;
	case WIFI_SECURITY_TYPE_SAE:
#if defined(CONFIG_ESP32_WIFI_ENABLE_WPA3_SAE)
		if (params->sae_password) {
			memcpy(wifi_config.sta.password, params->sae_password,
			       params->sae_password_length);
			wifi_config.sta.password[params->sae_password_length] = '\0';
		} else {
			memcpy(wifi_config.sta.password, params->psk, params->psk_length);
			wifi_config.sta.password[params->psk_length] = '\0';
		}
		data->status.security = WIFI_AUTH_WPA3_PSK;
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA3_PSK;
		wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
		break;
#else
		LOG_ERR("WPA3 not supported for STA mode. Enable "
			"CONFIG_ESP32_WIFI_ENABLE_WPA3_SAE");
		return -EINVAL;
#endif /* CONFIG_ESP32_WIFI_ENABLE_WPA3_SAE */
	default:
		LOG_ERR("Authentication method not supported");
		return -EIO;
	}

	if (params->channel == WIFI_CHANNEL_ANY) {
		wifi_config.sta.channel = 0U;
		data->status.channel = 0U;
	} else {
		wifi_config.sta.channel = params->channel;
		data->status.channel = params->channel;
	}

	ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	if (ret) {
		LOG_ERR("Failed to set Wi-Fi configuration (%d)", ret);
		return -EINVAL;
	}

	ret = esp_wifi_connect();
	if (ret) {
		LOG_ERR("Failed to connect to Wi-Fi access point (%d)", ret);
		return -EAGAIN;
	}

	return 0;
}

static int esp32_wifi_scan(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb)
{
	struct esp32_wifi_runtime *data = dev->data;
	int ret = 0;

	if (data->scan_cb != NULL) {
		LOG_INF("Scan callback in progress");
		return -EINPROGRESS;
	}

	data->scan_cb = cb;

	wifi_scan_config_t scan_config = { 0 };

	if (params) {
		/* The enum values are same, so, no conversion needed */
		scan_config.scan_type = params->scan_type;
	}

#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
	wifi_mode_t mode;

	esp_wifi_get_mode(&mode);
	if (mode == ESP32_WIFI_MODE_AP || mode == ESP32_WIFI_MODE_APSTA) {
		ret = esp_wifi_set_mode(ESP32_WIFI_MODE_APSTA);
	} else {
		ret = esp_wifi_set_mode(ESP32_WIFI_MODE_STA);
	}
#else
	ret = esp_wifi_set_mode(ESP32_WIFI_MODE_STA);
#endif

	if (ret) {
		LOG_ERR("Failed to set Wi-Fi mode (%d)", ret);
		return -EINVAL;
	}

	ret = esp_wifi_start();
	if (ret) {
		LOG_ERR("Failed to start Wi-Fi driver (%d)", ret);
		return -EAGAIN;
	}

	ret = esp_wifi_scan_start(&scan_config, false);
	if (ret != ESP_OK) {
		LOG_ERR("Failed to start Wi-Fi scanning");
		return -EAGAIN;
	}

	return 0;
};

static int esp32_wifi_ap_enable(const struct device *dev,
			 struct wifi_connect_req_params *params)
{
	struct esp32_wifi_runtime *data = dev->data;
	esp_err_t err = 0;

	/* Build Wi-Fi configuration for AP mode */
	wifi_config_t wifi_config = {
		.ap = {
			.max_connection = 5,
			.channel = params->channel == WIFI_CHANNEL_ANY ?
				0 : params->channel,
		},
	};

	memcpy(data->status.ssid, params->ssid, params->ssid_length);
	data->status.ssid[params->ssid_length] = '\0';

	strncpy((char *) wifi_config.ap.ssid, params->ssid, params->ssid_length);
	wifi_config.ap.ssid_len = params->ssid_length;

	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
		data->status.security = WIFI_AUTH_OPEN;
		wifi_config.ap.pmf_cfg.required = false;
		break;
	case WIFI_SECURITY_TYPE_PSK:
		strncpy((char *) wifi_config.ap.password, params->psk, params->psk_length);
		wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
		data->status.security = WIFI_AUTH_WPA2_PSK;
		wifi_config.ap.pmf_cfg.required = false;
		break;
	default:
		LOG_ERR("Authentication method not supported");
		return -EINVAL;
	}

	/* Start Wi-Fi in AP mode with configuration built above */
	wifi_mode_t mode;

	err = esp_wifi_get_mode(&mode);
	if (IS_ENABLED(CONFIG_ESP32_WIFI_AP_STA_MODE) &&
	    (mode == ESP32_WIFI_MODE_STA || mode == ESP32_WIFI_MODE_APSTA)) {
		err |= esp_wifi_set_mode(ESP32_WIFI_MODE_APSTA);
	} else {
		err |= esp_wifi_set_mode(ESP32_WIFI_MODE_AP);
	}
	if (err) {
		LOG_ERR("Failed to set Wi-Fi mode (%d)", err);
		return -EINVAL;
	}

	err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
	if (err) {
		LOG_ERR("Failed to set Wi-Fi configuration (%d)", err);
		return -EINVAL;
	}

	err = esp_wifi_start();
	if (err) {
		LOG_ERR("Failed to enable Wi-Fi AP mode");
		return -EAGAIN;
	}

	return 0;
};

static int esp32_wifi_ap_disable(const struct device *dev)
{
	int err = 0;
	wifi_mode_t mode;

	esp_wifi_get_mode(&mode);
	if (mode == ESP32_WIFI_MODE_APSTA) {
		err = esp_wifi_set_mode(ESP32_WIFI_MODE_STA);
		err |= esp_wifi_start();
	} else {
		err = esp_wifi_stop();
	}
	if (err) {
		LOG_ERR("Failed to disable Wi-Fi AP mode: (%d)", err);
		return -EAGAIN;
	}

	return 0;
};

static int esp32_wifi_status(const struct device *dev, struct wifi_iface_status *status)
{
	struct esp32_wifi_runtime *data = dev->data;
	wifi_mode_t mode;
	wifi_config_t conf;
	wifi_ap_record_t ap_info;

	switch (data->state) {
	case ESP32_STA_STOPPED:
	case ESP32_AP_STOPPED:
		status->state = WIFI_STATE_INACTIVE;
		break;
	case ESP32_STA_STARTED:
	case ESP32_AP_DISCONNECTED:
		status->state = WIFI_STATE_DISCONNECTED;
		break;
	case ESP32_STA_CONNECTING:
		status->state = WIFI_STATE_SCANNING;
		break;
	case ESP32_STA_CONNECTED:
	case ESP32_AP_CONNECTED:
		status->state = WIFI_STATE_COMPLETED;
		break;
	default:
		break;
	}

	strncpy(status->ssid, data->status.ssid, WIFI_SSID_MAX_LEN);
	status->ssid_len = strnlen(data->status.ssid, WIFI_SSID_MAX_LEN);
	status->ssid[status->ssid_len] = '\0';
	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->mfp = WIFI_MFP_DISABLE;

	if (esp_wifi_get_mode(&mode) == ESP_OK) {
		if (mode == ESP32_WIFI_MODE_STA) {
			wifi_phy_mode_t phy_mode;
			esp_err_t err;

			esp_wifi_get_config(ESP_IF_WIFI_STA, &conf);
			esp_wifi_sta_get_ap_info(&ap_info);

			status->iface_mode = WIFI_MODE_INFRA;
			status->channel = ap_info.primary;
			status->rssi = ap_info.rssi;
			memcpy(status->bssid, ap_info.bssid, WIFI_MAC_ADDR_LEN);

			err = esp_wifi_sta_get_negotiated_phymode(&phy_mode);
			if (err == ESP_OK) {
				if (phy_mode == WIFI_PHY_MODE_11B) {
					status->link_mode = WIFI_1;
				} else if (phy_mode == WIFI_PHY_MODE_11G) {
					status->link_mode = WIFI_3;
				} else if ((phy_mode == WIFI_PHY_MODE_HT20) ||
					   (phy_mode == WIFI_PHY_MODE_HT40)) {
					status->link_mode = WIFI_4;
				} else if (phy_mode == WIFI_PHY_MODE_HE20) {
					status->link_mode = WIFI_6;
				}
			}

			status->beacon_interval = conf.sta.listen_interval;
		} else if (mode == ESP32_WIFI_MODE_AP) {
			esp_wifi_get_config(ESP_IF_WIFI_AP, &conf);
			status->iface_mode = WIFI_MODE_AP;
			status->link_mode = WIFI_LINK_MODE_UNKNOWN;
			status->channel = conf.ap.channel;
			status->beacon_interval = conf.ap.beacon_interval;

		} else {
			status->iface_mode = WIFI_MODE_UNKNOWN;
			status->link_mode = WIFI_LINK_MODE_UNKNOWN;
			status->channel = 0;
		}
	}

	switch (data->status.security) {
	case WIFI_AUTH_OPEN:
		status->security = WIFI_SECURITY_TYPE_NONE;
		break;
	case WIFI_AUTH_WPA2_PSK:
		status->security = WIFI_SECURITY_TYPE_PSK;
		break;
	case WIFI_AUTH_WPA3_PSK:
		status->security = WIFI_SECURITY_TYPE_SAE;
		break;
	default:
		status->security = WIFI_SECURITY_TYPE_UNKNOWN;
	}

	return 0;
}

static void esp32_wifi_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct esp32_wifi_runtime *dev_data = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;

#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
	struct wifi_nm_instance *nm = wifi_nm_get_instance("esp32_wifi_nm");

	if (!esp32_wifi_iface_ap) {
		esp32_wifi_iface_ap = iface;
		dev_data->state = ESP32_AP_STOPPED;

		esp_read_mac(dev_data->mac_addr, ESP_MAC_WIFI_SOFTAP);
		wifi_nm_register_mgd_type_iface(nm, WIFI_TYPE_SAP, esp32_wifi_iface_ap);
	} else {
		esp32_wifi_iface = iface;
		dev_data->state = ESP32_STA_STOPPED;

		/* Start interface when we are actually connected with Wi-Fi network */
		esp_read_mac(dev_data->mac_addr, ESP_MAC_WIFI_STA);
		esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, eth_esp32_rx);
		wifi_nm_register_mgd_type_iface(nm, WIFI_TYPE_STA, esp32_wifi_iface);
	}
#else

	esp32_wifi_iface = iface;
	dev_data->state = ESP32_STA_STOPPED;

	/* Start interface when we are actually connected with Wi-Fi network */
	esp_read_mac(dev_data->mac_addr, ESP_MAC_WIFI_STA);
	esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, eth_esp32_rx);

#endif

	/* Assign link local address. */
	net_if_set_link_addr(iface, dev_data->mac_addr, 6, NET_LINK_ETHERNET);

	ethernet_init(iface);
	net_if_carrier_off(iface);
}

#if defined(CONFIG_NET_STATISTICS_WIFI)
static int esp32_wifi_stats(const struct device *dev, struct net_stats_wifi *stats)
{
	struct esp32_wifi_runtime *data = dev->data;

	stats->bytes.received = data->stats.bytes.received;
	stats->bytes.sent = data->stats.bytes.sent;
	stats->pkts.rx = data->stats.pkts.rx;
	stats->pkts.tx = data->stats.pkts.tx;
	stats->errors.rx = data->stats.errors.rx;
	stats->errors.tx = data->stats.errors.tx;
	stats->broadcast.rx = data->stats.broadcast.rx;
	stats->broadcast.tx = data->stats.broadcast.tx;
	stats->multicast.rx = data->stats.multicast.rx;
	stats->multicast.tx = data->stats.multicast.tx;
	stats->sta_mgmt.beacons_rx = data->stats.sta_mgmt.beacons_rx;
	stats->sta_mgmt.beacons_miss = data->stats.sta_mgmt.beacons_miss;

	return 0;
}
#endif

static int esp32_wifi_dev_init(const struct device *dev)
{
#if CONFIG_SOC_SERIES_ESP32S2 || CONFIG_SOC_SERIES_ESP32C3
	adc2_init_code_calibration();
#endif /* CONFIG_SOC_SERIES_ESP32S2 || CONFIG_SOC_SERIES_ESP32C3 */

	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	esp_err_t ret = esp_wifi_init(&config);

	if (ret == ESP_ERR_NO_MEM) {
		LOG_ERR("Not enough memory to initialize Wi-Fi.");
		LOG_ERR("Consider increasing CONFIG_HEAP_MEM_POOL_SIZE value.");
		return -ENOMEM;
	} else if (ret != ESP_OK) {
		LOG_ERR("Unable to initialize the Wi-Fi: %d", ret);
		return -EIO;
	}
	if (IS_ENABLED(CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4)) {
		net_mgmt_init_event_callback(&esp32_dhcp_cb, wifi_event_handler, DHCPV4_MASK);
		net_mgmt_add_event_callback(&esp32_dhcp_cb);
	}

	return 0;
}

static const struct wifi_mgmt_ops esp32_wifi_mgmt = {
	.scan		   = esp32_wifi_scan,
	.connect	   = esp32_wifi_connect,
	.disconnect	   = esp32_wifi_disconnect,
	.ap_enable	   = esp32_wifi_ap_enable,
	.ap_disable	   = esp32_wifi_ap_disable,
	.iface_status	   = esp32_wifi_status,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats	   = esp32_wifi_stats,
#endif
};

static const struct net_wifi_mgmt_offload esp32_api = {
	.wifi_iface.iface_api.init	  = esp32_wifi_init,
	.wifi_iface.send = esp32_wifi_send,
	.wifi_mgmt_api = &esp32_wifi_mgmt,
};

NET_DEVICE_DT_INST_DEFINE(0,
		esp32_wifi_dev_init, NULL,
		&esp32_data, NULL, CONFIG_WIFI_INIT_PRIORITY,
		&esp32_api, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

#if defined(CONFIG_ESP32_WIFI_AP_STA_MODE)
NET_DEVICE_DT_INST_DEFINE(1,
		NULL, NULL,
		&esp32_ap_sta_data, NULL, CONFIG_WIFI_INIT_PRIORITY,
		&esp32_api, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

DEFINE_WIFI_NM_INSTANCE(esp32_wifi_nm, &esp32_wifi_mgmt);

CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));
#endif
