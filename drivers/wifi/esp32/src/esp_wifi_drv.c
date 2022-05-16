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
#include <zephyr/device.h>
#include <soc.h>
#include <ethernet/eth_stats.h>
#include "esp_networking_priv.h"
#include "esp_private/wifi.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_wpa.h"

/* use global iface pointer to support any ethernet driver */
/* necessary for wifi callback functions */
static struct net_if *esp32_wifi_iface;

struct esp32_wifi_runtime {
	struct net_if *iface;
	uint8_t mac_addr[6];
	bool tx_err;
	uint32_t tx_word;
	int tx_pos;
	uint8_t frame_buf[NET_ETH_MAX_FRAME_SIZE];
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

static void esp_wifi_event_task(void);

K_MSGQ_DEFINE(esp_wifi_msgq, sizeof(system_event_t), 10, 4);
K_THREAD_STACK_DEFINE(esp_wifi_event_stack, CONFIG_ESP32_WIFI_EVENT_TASK_STACK_SIZE);
static struct k_thread esp_wifi_event_thread;

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
		return ESP_FAIL;
	}

	memcpy(&evt.event_info, event_data, event_data_size);
	k_msgq_put(&esp_wifi_msgq, &evt, K_FOREVER);
	return ESP_OK;
}

static int eth_esp32_send(const struct device *dev, struct net_pkt *pkt)
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
		return ESP_FAIL;
	}

	pkt = net_pkt_rx_alloc_with_buffer(esp32_wifi_iface, len,
			AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to get net buffer");
		return ESP_FAIL;
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
	return ESP_OK;

pkt_unref:
	net_pkt_unref(pkt);
	return ESP_FAIL;
}

static void esp_wifi_event_task(void)
{
	system_event_t evt;

	while (1) {
		k_msgq_get(&esp_wifi_msgq, &evt, K_FOREVER);

		switch (evt.event_id) {
		case ESP32_WIFI_EVENT_STA_START:
			LOG_INF("WIFI_EVENT_STA_START");
			net_if_up(esp32_wifi_iface);
			break;
		case ESP32_WIFI_EVENT_STA_STOP:
			LOG_INF("WIFI_EVENT_STA_STOP");
			net_if_down(esp32_wifi_iface);
			break;
		case ESP32_WIFI_EVENT_STA_CONNECTED:
			LOG_INF("WIFI_EVENT_STA_CONNECTED");
			net_eth_carrier_on(esp32_wifi_iface);
			break;
		case ESP32_WIFI_EVENT_STA_DISCONNECTED:
			LOG_INF("WIFI_EVENT_STA_DISCONNECTED");
			net_eth_carrier_off(esp32_wifi_iface);

			if (IS_ENABLED(CONFIG_ESP32_WIFI_STA_RECONNECT)) {
				esp_wifi_connect();
			}
			break;
		default:
			break;
		}
	}
}

static void eth_esp32_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct esp32_wifi_runtime *dev_data = dev->data;

	dev_data->iface = iface;
	esp32_wifi_iface = iface;

	/* Start interface when we are actually connected with WiFi network */
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	esp_read_mac(dev_data->mac_addr, ESP_MAC_WIFI_STA);

	/* Assign link local address. */
	net_if_set_link_addr(iface,
			dev_data->mac_addr, 6, NET_LINK_ETHERNET);

	ethernet_init(iface);

	esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, eth_esp32_rx);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_esp32_stats(const struct device *dev)
{
	struct esp32_wifi_runtime *data = dev->data;

	return &(data->stats);
}
#endif

static int eth_esp32_dev_init(const struct device *dev)
{
	esp_timer_init();

	k_tid_t tid = k_thread_create(&esp_wifi_event_thread, esp_wifi_event_stack,
			CONFIG_ESP32_WIFI_EVENT_TASK_STACK_SIZE,
			(k_thread_entry_t)esp_wifi_event_task, NULL, NULL, NULL,
			CONFIG_ESP32_WIFI_EVENT_TASK_PRIO, K_INHERIT_PERMS,
			K_NO_WAIT);

	k_thread_name_set(tid, "esp_event");

	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
	esp_err_t ret = esp_wifi_init(&config);

	ret |= esp_supplicant_init();
	ret |= esp_wifi_start();

	if (IS_ENABLED(CONFIG_ESP32_WIFI_STA_AUTO)) {
		wifi_config_t wifi_config = {
			.sta = {
				.ssid = CONFIG_ESP32_WIFI_SSID,
				.password = CONFIG_ESP32_WIFI_PASSWORD,
			},
		};

		ret = esp_wifi_set_mode(WIFI_MODE_STA);
		ret |= esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
		ret |= esp_wifi_connect();
	}

	if (ret != ESP_OK) {
		LOG_ERR("Connect failed");
	}

	return ret;
}


static struct esp32_wifi_runtime eth_data;

static const struct ethernet_api eth_esp32_apis = {
	.iface_api.init	= eth_esp32_init,
	.send =  eth_esp32_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_esp32_stats,
#endif
};

NET_DEVICE_DT_INST_DEFINE(0,
		eth_esp32_dev_init, NULL,
		&eth_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		&eth_esp32_apis, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
