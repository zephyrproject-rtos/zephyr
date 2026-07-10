/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Private interface between the core Wi-Fi driver and the mesh support code.
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ESP32_ESP_WIFI_MESH_PRIV_H_
#define ZEPHYR_DRIVERS_WIFI_ESP32_ESP_WIFI_MESH_PRIV_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include "esp_event.h"
#include "esp_mesh.h"

/**
 * @brief Read the vendor mesh routing table under the shared lock.
 *
 * The vendor routing-table read is shared by the mesh send path, the
 * routing-table query, and the IP-over-mesh downlink broadcast, and it fills a
 * caller-provided buffer, so this serializes those reads behind one lock owned
 * by the mesh core.
 *
 * @param buf      output array receiving the routing-table entries.
 * @param buf_size size of @p buf in bytes.
 * @param count    output set to the number of entries written.
 */
void esp_wifi_mesh_read_routing_table(mesh_addr_t *buf, size_t buf_size, int *count);

/**
 * @brief Query whether the mesh stack is currently active.
 *
 * The core driver event handlers consult this to skip station/softAP state
 * transitions the mesh stack owns while it drives the interfaces directly.
 *
 * @return true while the mesh stack manages the Wi-Fi interfaces.
 */
bool esp_wifi_mesh_is_active(void);

/**
 * @brief Dispatch a posted event to the mesh event handler registry.
 *
 * Called from esp_event_post() after the core driver handler. Delivers the
 * event to every matching handler the mesh stack and application registered.
 */
void esp_wifi_mesh_dispatch_event(esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif /* ZEPHYR_DRIVERS_WIFI_ESP32_ESP_WIFI_MESH_PRIV_H_ */
