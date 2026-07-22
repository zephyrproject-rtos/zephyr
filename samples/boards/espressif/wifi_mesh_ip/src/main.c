/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/drivers/wifi/esp_wifi_mesh.h>

#include "mesh_bridge.h"

LOG_MODULE_REGISTER(wifi_mesh_ip, LOG_LEVEL_INF);

/*
 * IP-over-mesh sample.
 *
 * Every node runs Wi-Fi in combined AP and station mode and self-organizes
 * into a mesh; only the root reaches the router. IP is carried over the mesh
 * so sockets run unmodified on every node: the root serves DHCP and NATs child
 * traffic out its station uplink, and a child reaches the external network
 * through the root. The Wi-Fi driver performs the mesh bring-up and provides
 * the Ethernet-over-mesh transport; this sample wires the IP layer on top.
 */

static void mesh_event(enum esp_wifi_mesh_event event, const struct esp_wifi_mesh_event_info *info)
{
	switch (event) {
	case ESP_WIFI_MESH_EVENT_STARTED:
		LOG_INF("mesh started, layer %d", info->layer);
		break;
	case ESP_WIFI_MESH_EVENT_STOPPED:
		LOG_INF("mesh stopped");
		break;
	case ESP_WIFI_MESH_EVENT_PARENT_CONNECTED:
		LOG_INF("parent connected, layer %d%s", info->layer,
			info->is_root ? " (root)" : "");
		if (info->is_root) {
			struct net_if *sta = net_if_get_first_wifi();

			if (sta != NULL) {
				mesh_bridge_setup_root(sta);
			}
		} else {
			mesh_bridge_setup_child();
		}
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

	LOG_INF("Wi-Fi mesh starting");
	return 0;
}
