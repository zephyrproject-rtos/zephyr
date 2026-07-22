/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/drivers/wifi/esp_wifi_mesh.h>

#include "mesh_demo.h"

LOG_MODULE_REGISTER(wifi_mesh, LOG_LEVEL_INF);

/*
 * Self-organizing Wi-Fi mesh sample.
 *
 * Every node runs Wi-Fi in combined AP and station mode. The station side
 * links upward to a parent, the softAP side accepts children. Nodes elect a
 * root among themselves; only the root reaches the router (external network).
 * The Wi-Fi driver performs the mesh bring-up; this sample reacts to mesh
 * events, logs received mesh messages, and drives the mesh from a shell.
 */

#define MESH_TX_PERIOD     K_SECONDS(5)
#define MESH_RX_TIMEOUT_MS 1000

static uint8_t rx_buf[128];

bool mesh_running;
bool mesh_autotx;

static void mesh_event(enum esp_wifi_mesh_event event, const struct esp_wifi_mesh_event_info *info)
{
	switch (event) {
	case ESP_WIFI_MESH_EVENT_STARTED:
		mesh_running = true;
		LOG_INF("mesh started, layer %d", info->layer);
		break;
	case ESP_WIFI_MESH_EVENT_STOPPED:
		mesh_running = false;
		LOG_INF("mesh stopped");
		break;
	case ESP_WIFI_MESH_EVENT_PARENT_CONNECTED:
		LOG_INF("parent connected, layer %d%s", info->layer,
			info->is_root ? " (root)" : "");
		break;
	case ESP_WIFI_MESH_EVENT_PARENT_DISCONNECTED:
		LOG_INF("parent disconnected, reason %d", info->reason);
		break;
	case ESP_WIFI_MESH_EVENT_NO_PARENT_FOUND:
		LOG_INF("no parent found, scanning");
		break;
	case ESP_WIFI_MESH_EVENT_FIND_NETWORK:
		LOG_INF("find network");
		break;
	case ESP_WIFI_MESH_EVENT_CHILD_CONNECTED:
		LOG_INF("child connected");
		break;
	case ESP_WIFI_MESH_EVENT_CHILD_DISCONNECTED:
		LOG_INF("child disconnected");
		break;
	case ESP_WIFI_MESH_EVENT_ROUTING_TABLE_CHANGE:
		LOG_INF("routing table size %d", info->routing_table_size);
		break;
	case ESP_WIFI_MESH_EVENT_TODS_REACHABLE:
		LOG_INF("root external network reachable");
		break;
	case ESP_WIFI_MESH_EVENT_TODS_UNREACHABLE:
		LOG_INF("root external network unreachable");
		break;
	default:
		break;
	}
}

static void mesh_rx_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		size_t len = sizeof(rx_buf);

		if (!mesh_running) {
			k_sleep(K_MSEC(200));
			continue;
		}

		if (esp_wifi_mesh_recv(rx_buf, &len, MESH_RX_TIMEOUT_MS) == 0) {
			/* Terminate before logging: a peer may send a payload
			 * that is not NUL-terminated or fills the whole buffer.
			 */
			rx_buf[MIN(len, sizeof(rx_buf) - 1)] = '\0';
			LOG_INF("rx %d bytes: %s", (int)len, (char *)rx_buf);
		}
	}
}

/*
 * Optional periodic sender, off by default and toggled with "mesh autotx".
 * When enabled it sends a "hello seq=N" message every few seconds so the mesh
 * can be exercised without typing "mesh send" by hand.
 */
static void mesh_tx_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint8_t tx_buf[64];
	uint32_t seq = 0;

	while (1) {
		k_sleep(MESH_TX_PERIOD);

		if (!mesh_running || !mesh_autotx) {
			continue;
		}

		int len = snprintk((char *)tx_buf, sizeof(tx_buf), "hello seq=%u", seq++) + 1;

		esp_wifi_mesh_send(tx_buf, len);
	}
}

K_THREAD_DEFINE(mesh_rx_id, 4096, mesh_rx_task, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(mesh_tx_id, 4096, mesh_tx_task, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	if (net_if_get_first_wifi() == NULL) {
		LOG_ERR("no Wi-Fi interface found");
		return -ENODEV;
	}

	esp_wifi_mesh_event_cb_register(mesh_event);

	if (esp_wifi_mesh_start() != 0) {
		LOG_ERR("Wi-Fi mesh start failed");
		return -EIO;
	}

	LOG_INF("Wi-Fi mesh starting, use the \"mesh\" shell commands");
	return 0;
}
