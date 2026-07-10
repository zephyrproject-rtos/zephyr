/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP32 Wi-Fi mesh public API.
 *
 * Zephyr-native interface to the ESP Wi-Fi mesh stack: mesh bring-up, the
 * application event callback, the raw send/receive path, and status and
 * routing-table queries. It hides the vendor mesh headers so applications
 * depend only on this API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_WIFI_MESH_H_
#define ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_WIFI_MESH_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Wi-Fi mesh events delivered to the application callback.
 *
 * These abstract the underlying vendor mesh events into a stable Zephyr-native
 * set so the application does not depend on vendor mesh headers.
 */
enum esp_wifi_mesh_event {
	/** The mesh stack has started. */
	ESP_WIFI_MESH_EVENT_STARTED,
	/** The mesh stack has stopped. */
	ESP_WIFI_MESH_EVENT_STOPPED,
	/** This node connected to a parent (see is_root in the info). */
	ESP_WIFI_MESH_EVENT_PARENT_CONNECTED,
	/** This node lost its parent. */
	ESP_WIFI_MESH_EVENT_PARENT_DISCONNECTED,
	/** A child node connected to this node. */
	ESP_WIFI_MESH_EVENT_CHILD_CONNECTED,
	/** A child node disconnected from this node. */
	ESP_WIFI_MESH_EVENT_CHILD_DISCONNECTED,
	/** No parent was found during the last scan. */
	ESP_WIFI_MESH_EVENT_NO_PARENT_FOUND,
	/** The node started looking for a network to join. */
	ESP_WIFI_MESH_EVENT_FIND_NETWORK,
	/** The routing table changed (see routing_table_size in the info). */
	ESP_WIFI_MESH_EVENT_ROUTING_TABLE_CHANGE,
	/** The root external-network (toDS) path became reachable. */
	ESP_WIFI_MESH_EVENT_TODS_REACHABLE,
	/** The root external-network (toDS) path became unreachable. */
	ESP_WIFI_MESH_EVENT_TODS_UNREACHABLE,
};

/**
 * @brief Information delivered with a mesh event.
 */
struct esp_wifi_mesh_event_info {
	/** Current mesh layer (tree depth); -1 while unattached. */
	int layer;
	/** True if this node is the mesh root. */
	bool is_root;
	/** Current routing table size (valid for routing-table events). */
	int routing_table_size;
	/** Disconnect reason (valid for disconnect events). */
	int reason;
};

/**
 * @brief Application mesh event callback.
 *
 * @param event the event that occurred.
 * @param info event details.
 */
typedef void (*esp_wifi_mesh_event_cb_t)(enum esp_wifi_mesh_event event,
					 const struct esp_wifi_mesh_event_info *info);

/**
 * @brief Register the application mesh event callback.
 *
 * The callback runs on the shared Wi-Fi event task and must not block. Its
 * stack is CONFIG_ESP32_WIFI_EVENT_TASK_STACK_SIZE; heavy work (bringing up the
 * IP stack, DHCP, NAT) should either run on the application's own thread or the
 * event task stack should be sized for it. Only one callback is supported;
 * registering again replaces the previous one.
 *
 * @param cb callback to invoke on mesh events, or NULL to unregister.
 */
void esp_wifi_mesh_event_cb_register(esp_wifi_mesh_event_cb_t cb);

/**
 * @brief Start the Wi-Fi mesh stack.
 *
 * Performs the full mesh bring-up: puts Wi-Fi in AP+STA mode, initializes and
 * configures the mesh stack from the driver Kconfig options (mesh id, channel,
 * router and softAP credentials, tree limits), disables power save, and starts
 * the mesh.
 *
 * The node role follows the WIFI_ESP32_MESH_ROLE choice: the auto role runs
 * self-organized election (a router is required); WIFI_ESP32_MESH_FORCE_ROOT
 * designates this node as the fixed root and stops its self-organized parent
 * search; WIFI_ESP32_MESH_FORCE_CHILD keeps a node from becoming root.
 *
 * A router is required. The tree forms around the root's association to the
 * router, and children auto-attach through the self-organized parent search.
 * A fully router-less mesh (no router at all) needs each child to be given an
 * explicit parent, which this driver does not do, so a child does not attach
 * to a fixed root without a router.
 *
 * @note Mesh is a boot-time mode with no runtime teardown. Once started it
 *       stays resident for the lifetime of the node; there is no stop or
 *       deinit entry point, and the normal station/softAP wifi_mgmt operations
 *       (connect, disconnect, ap_enable, scan) are rejected while it runs.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the auto role is used without a router SSID.
 * @retval -EIO on a mesh stack error.
 */
int esp_wifi_mesh_start(void);

#if !defined(CONFIG_WIFI_ESP32_MESH_IP)
/**
 * @brief Send a data buffer over the mesh.
 *
 * On a non-root node the buffer is routed up to the root. On the root it is
 * sent to every node in the routing table. This is a convenience path for
 * application messaging that does not use the IP-over-mesh interface.
 *
 * @note Not available with CONFIG_WIFI_ESP32_MESH_IP. The IP interface and
 *       this raw path would both drain the single mesh receive queue with no
 *       demux, so frames would split non-deterministically between the two
 *       readers. They are kept mutually exclusive until the receive path
 *       demultiplexes on the frame protocol tag.
 *
 * @param data buffer to send.
 * @param len number of bytes to send.
 *
 * @retval 0 on success (or partial success on the root).
 * @retval -EIO on a mesh send error.
 */
int esp_wifi_mesh_send(const uint8_t *data, size_t len);

/**
 * @brief Receive a data buffer from the mesh.
 *
 * Blocks up to @p timeout_ms for a mesh frame addressed to this node.
 *
 * @note Not available with CONFIG_WIFI_ESP32_MESH_IP (see
 *       esp_wifi_mesh_send()).
 *
 * @param data buffer to receive into.
 * @param len in: buffer size; out: received length.
 * @param timeout_ms receive timeout in milliseconds.
 *
 * @retval 0 on success, with @p len updated to the received size.
 * @retval -EAGAIN if no frame arrived before the timeout.
 */
int esp_wifi_mesh_recv(uint8_t *data, size_t *len, int timeout_ms);
#endif /* !CONFIG_WIFI_ESP32_MESH_IP */

/**
 * @brief Current mesh status snapshot.
 */
struct esp_wifi_mesh_status {
	/** Current mesh layer (tree depth); -1 while unattached. */
	int layer;
	/** True if this node is the mesh root. */
	bool is_root;
	/** Number of nodes in the routing table (root: whole sub-network). */
	int routing_table_size;
	/** True once the root announced external-network reachability. */
	bool tods_reachable;
};

/**
 * @brief Read the current mesh status.
 *
 * @param status output filled with the current mesh state.
 */
void esp_wifi_mesh_get_status(struct esp_wifi_mesh_status *status);

/**
 * @brief A mesh node address (a 6-byte MAC).
 */
struct esp_wifi_mesh_addr {
	/** 6-byte MAC address of the mesh node. */
	uint8_t addr[6];
};

/**
 * @brief Read the mesh routing table node addresses.
 *
 * On the root the table spans the whole sub-network; on an internal node it
 * holds the node's own sub-tree.
 *
 * @param macs   output array receiving the node addresses.
 * @param max    number of entries @p macs can hold.
 * @param count  output set to the number of entries written.
 *
 * @retval 0 on success.
 * @retval -EINVAL on a NULL argument.
 */
int esp_wifi_mesh_get_routing_table(struct esp_wifi_mesh_addr *macs, size_t max, size_t *count);

/**
 * @brief Announce the root external-network (toDS) reachability to the mesh.
 *
 * Called on the root once its uplink is up (or down) so the mesh admits or
 * stops admitting child traffic bound for the external network.
 *
 * @param reachable true if the root can forward toward the external network.
 */
void esp_wifi_mesh_set_tods_reachable(bool reachable);

/**
 * @brief Deliver root station uplink frames to the station interface.
 *
 * The mesh stack installs its own station receive callback. On the root, the
 * station also carries the uplink to the router, so this restores the driver
 * receive path for that interface. Call once the node becomes the mesh root.
 */
void esp_wifi_mesh_bind_sta_rx(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_WIFI_ESP_WIFI_MESH_H_ */
