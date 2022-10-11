/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_wifi

#define _POSIX_C_SOURCE 200809

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_wifi, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/device.h>
#include <soc.h>
#include <ethernet/eth_stats.h>
#include "esp_networking_priv.h"
#include "esp_private/wifi.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_wpa.h"

#define DHCPV4_MASK (NET_EVENT_IPV4_DHCP_BOUND | NET_EVENT_IPV4_DHCP_STOP)

/* use global iface pointer to support any ethernet driver */
/* necessary for wifi callback functions */
static struct net_if *esp32_wifi_iface;
static struct esp32_wifi_runtime esp32_data;

enum esp32_state_flag {
	ESP32_STA_STOPPED,
	ESP32_STA_STARTED,
	ESP32_STA_CONNECTING,
	ESP32_STA_CONNECTED,
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
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
	struct esp32_wifi_status status;
	scan_result_cb_t scan_cb;
	uint8_t state;
};

static void esp_wifi_event_task(void);

K_MSGQ_DEFINE(esp_wifi_msgq, sizeof(system_event_t), 10, 4);
K_THREAD_STACK_DEFINE(esp_wifi_event_stack, CONFIG_ESP32_WIFI_EVENT_TASK_STACK_SIZE);
static struct k_thread esp_wifi_event_thread;

static struct net_mgmt_event_callback esp32_dhcp_cb;

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			       struct net_if *iface)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		wifi_mgmt_raise_connect_result_event(esp32_wifi_iface, 0);
		break;
	default:
		break;
	}
}

/* internal wifi library callback function */
esp_err_t esp_event_send_internal(esp_event_base_t event_base,
				  int32_t event_id,
				  void *event_data,
				  size_t event_data_size,
				  uint32_t ticks_to_wait)
{
	system_event_t evt = {
		.event_id = event_id,
	};

	if (event_data_size > sizeof(evt.event_info)) {
		LOG_ERR("MSG %d wont find %d > %d",
			event_id, event_data_size, sizeof(evt.event_info));
		return -EIO;
	}

	memcpy(&evt.event_info, event_data, event_data_size);
	k_msgq_put(&esp_wifi_msgq, &evt, K_FOREVER);
	return 0;
}

static int esp32_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	struct esp32_wifi_runtime *data = dev->data;
	const int pkt_len = net_pkt_get_len(pkt);

	/* Read the packet payload */
	if (net_pkt_read(pkt, data->frame_buf, pkt_len) < 0) {
		return -EIO;
	}

	/* Enqueue packet for transmission */
	esp_wifi_internal_tx(ESP_IF_WIFI_STA, (void *)data->frame_buf, pkt_len);

	LOG_DBG("pkt sent %p len %d", pkt, pkt_len);

	return 0;
}

static esp_err_t eth_esp32_rx(void *buffer, uint16_t len, void *eb)
{
	struct net_pkt *pkt;

	if (esp32_wifi_iface == NULL) {
		LOG_ERR("network interface unavailable");
		return -EIO;
	}

	pkt = net_pkt_rx_alloc_with_buffer(esp32_wifi_iface, len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to get net buffer");
		return -EIO;
	}

	if (net_pkt_write(pkt, buffer, len) < 0) {
		LOG_ERR("Failed to write pkt");
		goto pkt_unref;
	}

	if (net_recv_data(esp32_wifi_iface, pkt) < 0) {
		LOG_ERR("Failed to push received data");
		goto pkt_unref;
	}

	esp_wifi_internal_free_rx_buffer(eb);
	return 0;

pkt_unref:
	net_pkt_unref(pkt);
	return -EIO;
}

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
			res.security = WIFI_SECURITY_TYPE_NONE;
			if (ap_list_buffer[k].authmode > WIFI_AUTH_OPEN) {
				res.security = WIFI_SECURITY_TYPE_PSK;
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

static void esp_wifi_handle_connect_event(void)
{
	esp32_data.state = ESP32_STA_CONNECTED;
	if (IS_ENABLED(CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4)) {
		net_dhcpv4_start(esp32_wifi_iface);
	} else {
		wifi_mgmt_raise_connect_result_event(esp32_wifi_iface, 0);
	}
}

static void esp_wifi_handle_disconnect_event(void)
{
	if (esp32_data.state == ESP32_STA_CONNECTED) {
		if (IS_ENABLED(CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4)) {
			net_dhcpv4_stop(esp32_wifi_iface);
		}
		wifi_mgmt_raise_disconnect_result_event(esp32_wifi_iface, 0);
	} else {
		wifi_mgmt_raise_disconnect_result_event(esp32_wifi_iface, -1);
	}

	if (IS_ENABLED(CONFIG_ESP32_WIFI_STA_RECONNECT)) {
		esp32_data.state = ESP32_STA_CONNECTING;
		esp_wifi_connect();
	} else {
		esp32_data.state = ESP32_STA_STARTED;
	}
}

static void esp_wifi_event_task(void)
{
	system_event_t evt;
	uint8_t s_con_cnt = 0;

	while (1) {
		k_msgq_get(&esp_wifi_msgq, &evt, K_FOREVER);

		switch (evt.event_id) {
		case ESP32_WIFI_EVENT_STA_START:
			esp32_data.state = ESP32_STA_STARTED;
			net_eth_carrier_on(esp32_wifi_iface);
			break;
		case ESP32_WIFI_EVENT_STA_STOP:
			esp32_data.state = ESP32_STA_STOPPED;
			net_eth_carrier_off(esp32_wifi_iface);
			break;
		case ESP32_WIFI_EVENT_STA_CONNECTED:
			esp_wifi_handle_connect_event();
			break;
		case ESP32_WIFI_EVENT_STA_DISCONNECTED:
			esp_wifi_handle_disconnect_event();
			break;
		case ESP32_WIFI_EVENT_SCAN_DONE:
			scan_done_handler();
			break;
		case ESP32_WIFI_EVENT_AP_STOP:
			esp32_data.state = ESP32_AP_STOPPED;
			break;
		case ESP32_WIFI_EVENT_AP_STACONNECTED:
			esp32_data.state = ESP32_AP_CONNECTED;
			if (!s_con_cnt) {
				esp_wifi_internal_reg_rxcb(WIFI_IF_AP, eth_esp32_rx);
			}
			s_con_cnt++;
			break;
		case ESP32_WIFI_EVENT_AP_STADISCONNECTED:
			esp32_data.state = ESP32_AP_DISCONNECTED;
			s_con_cnt--;
			if (!s_con_cnt) {
				esp_wifi_internal_reg_rxcb(WIFI_IF_AP, NULL);
			}
			break;
		default:
			break;
		}
	}
}

static int esp32_wifi_disconnect(const struct device *dev)
{
	struct esp32_wifi_runtime *data = dev->data;
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
	int ret;

	if (data->state == ESP32_STA_CONNECTING || data->state == ESP32_STA_CONNECTED) {
		wifi_mgmt_raise_connect_result_event(esp32_wifi_iface, -1);
		return -EALREADY;
	}

	if (data->state != ESP32_STA_STARTED) {
		LOG_ERR("Wi-Fi not in station mode");
		wifi_mgmt_raise_connect_result_event(esp32_wifi_iface, -1);
		return -EIO;
	}

	data->state = ESP32_STA_CONNECTING;

	memcpy(data->status.ssid, params->ssid, params->ssid_length);
	data->status.ssid[params->ssid_length] = '\0';

	wifi_config_t wifi_config;

	memset(&wifi_config, 0, sizeof(wifi_config_t));

	memcpy(wifi_config.sta.ssid, params->ssid, params->ssid_length);
	wifi_config.sta.ssid[params->ssid_length] = '\0';

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		memcpy(wifi_config.sta.password, params->psk, params->psk_length);
		wifi_config.sta.password[params->psk_length] = '\0';
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
		data->status.security = WIFI_AUTH_WPA2_PSK;
	} else if (params->security == WIFI_SECURITY_TYPE_NONE) {
		wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
		data->status.security = WIFI_AUTH_OPEN;
	} else {
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

	wifi_config.sta.pmf_cfg.capable = true;
	wifi_config.sta.pmf_cfg.required = false;

	ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	ret |= esp_wifi_set_mode(ESP32_WIFI_MODE_STA);
	ret |= esp_wifi_connect();

	if (ret != ESP_OK) {
		LOG_ERR("Failed to connect to Wi-Fi access point");
		return -EAGAIN;
	}

	return 0;
}

static int esp32_wifi_scan(const struct device *dev, scan_result_cb_t cb)
{
	struct esp32_wifi_runtime *data = dev->data;
	int ret = 0;

	if (data->scan_cb != NULL) {
		LOG_INF("Scan callback in progress");
		return -EINPROGRESS;
	}

	data->scan_cb = cb;

	wifi_scan_config_t scan_config = { 0 };

	ret = esp_wifi_set_mode(ESP32_WIFI_MODE_STA);
	ret |= esp_wifi_scan_start(&scan_config, false);

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
	esp_err_t ret = 0;

	/* Build Wi-Fi configuration for AP mode */
	wifi_config_t wifi_config = {
		.ap = {
			.max_connection = 5,
			.channel = params->channel
		},
	};

	memcpy(data->status.ssid, params->ssid, params->ssid_length);
	data->status.ssid[params->ssid_length] = '\0';

	strncpy((char *) wifi_config.ap.ssid, params->ssid, params->ssid_length);

	if (params->psk_length == 0) {
		memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
		data->status.security = WIFI_AUTH_OPEN;
	} else {
		strncpy((char *) wifi_config.ap.password, params->psk, params->psk_length);
		wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
		data->status.security = WIFI_AUTH_WPA2_PSK;
	}

	/* Start Wi-Fi in AP mode with configuration built above */
	ret = esp_wifi_set_mode(ESP32_WIFI_MODE_AP);
	ret |= esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
	ret |= esp_wifi_start();
	if (ret != ESP_OK) {
		LOG_ERR("Failed to enable Wi-Fi AP mode");
		return -EAGAIN;
	}

	return 0;
};

static int esp32_wifi_ap_disable(const struct device *dev)
{
	esp_err_t ret = esp_wifi_set_mode(ESP32_WIFI_MODE_NULL);

	ret |= esp_wifi_start();
	if (ret != ESP_OK) {
		LOG_ERR("Failed to disable Wi-Fi AP mode");
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

	strcpy(status->ssid, data->status.ssid);
	status->ssid_len = strlen(data->status.ssid);
	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->mfp = WIFI_MFP_DISABLE;

	if (esp_wifi_get_mode(&mode) == ESP_OK) {
		if (mode == ESP32_WIFI_MODE_STA) {
			esp_wifi_get_config(ESP_IF_WIFI_STA, &conf);
			esp_wifi_sta_get_ap_info(&ap_info);
			status->iface_mode = WIFI_MODE_INFRA;
			status->channel = ap_info.primary;
			status->rssi = ap_info.rssi;
			memcpy(status->bssid, ap_info.bssid, WIFI_MAC_ADDR_LEN);

			if (ap_info.phy_11n) {
				status->link_mode = WIFI_4;
			} else if (ap_info.phy_11g) {
				status->link_mode |= WIFI_3;
			} else if (ap_info.phy_11b) {
				status->link_mode = WIFI_1;
			}

		} else if (mode == ESP32_WIFI_MODE_AP) {
			esp_wifi_get_config(ESP_IF_WIFI_AP, &conf);
			status->iface_mode = WIFI_MODE_AP;
			status->link_mode = WIFI_LINK_MODE_UNKNOWN;
			status->channel = conf.ap.channel;
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
	default:
		status->security = WIFI_SECURITY_TYPE_UNKNOWN;
	}

	return 0;
}

static void esp32_wifi_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct esp32_wifi_runtime *dev_data = dev->data;

	esp32_wifi_iface = iface;
	dev_data->state = ESP32_STA_STOPPED;

	/* Start interface when we are actually connected with Wi-Fi network */
	esp_read_mac(dev_data->mac_addr, ESP_MAC_WIFI_STA);

	/* Assign link local address. */
	net_if_set_link_addr(iface, dev_data->mac_addr, 6, NET_LINK_ETHERNET);

	ethernet_init(iface);
	net_if_carrier_off(iface);

	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	esp_err_t ret = esp_wifi_init(&config);

	esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, eth_esp32_rx);

	ret |= esp_wifi_set_mode(ESP32_WIFI_MODE_STA);
	ret |= esp_wifi_start();

	if (ret != ESP_OK) {
		LOG_ERR("Failed to start Wi-Fi driver");
	}
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *esp32_wifi_stats(const struct device *dev)
{
	struct esp32_wifi_runtime *data = dev->data;

	return &(data->stats);
}
#endif

static int esp32_wifi_dev_init(const struct device *dev)
{
	esp_timer_init();

	k_tid_t tid = k_thread_create(&esp_wifi_event_thread, esp_wifi_event_stack,
			CONFIG_ESP32_WIFI_EVENT_TASK_STACK_SIZE,
			(k_thread_entry_t)esp_wifi_event_task, NULL, NULL, NULL,
			CONFIG_ESP32_WIFI_EVENT_TASK_PRIO, K_INHERIT_PERMS,
			K_NO_WAIT);

	k_thread_name_set(tid, "esp_event");

	if (IS_ENABLED(CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4)) {
		net_mgmt_init_event_callback(&esp32_dhcp_cb, wifi_event_handler, DHCPV4_MASK);
		net_mgmt_add_event_callback(&esp32_dhcp_cb);
	}

	return 0;
}

static const struct net_wifi_mgmt_offload esp32_api = {
	.wifi_iface.iface_api.init = esp32_wifi_init,
	.wifi_iface.send = esp32_wifi_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.wifi_iface.get_stats = esp32_wifi_stats,
 #endif
	.scan				 = esp32_wifi_scan,
	.connect			 = esp32_wifi_connect,
	.disconnect			 = esp32_wifi_disconnect,
	.ap_enable			 = esp32_wifi_ap_enable,
	.ap_disable			 = esp32_wifi_ap_disable,
	.iface_status		 = esp32_wifi_status,
};

NET_DEVICE_DT_INST_DEFINE(0,
		esp32_wifi_dev_init, NULL,
		&esp32_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		&esp32_api, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
