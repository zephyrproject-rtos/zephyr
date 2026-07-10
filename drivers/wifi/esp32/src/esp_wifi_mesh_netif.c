/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ethernet-over-mesh network interface for the Wi-Fi mesh transport. It carries
 * Ethernet frames as the mesh payload so the Zephyr network stack (IP, DHCP,
 * NAT, sockets) runs unmodified on top of the mesh.
 *
 *   TX:  net_if send  ->  read Ethernet frame from net_pkt  ->  esp_mesh_send
 *   RX:  esp_mesh_recv ->  wrap frame in net_pkt            ->  net_recv_data
 *
 * Root node:    frames from children arrive toDS and are handed to the IP
 *               stack, which NATs/forwards them out the station interface.
 * Non-root:     frames from the IP stack are sent toDS to the root; frames
 *               from the root arrive and are handed up.
 *
 * The interface is a plain Ethernet net_if; all IP-level policy (addresses,
 * DHCP, NAT, routes) is left to the application.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/wifi/esp_wifi_mesh.h>

#include "esp_mesh.h"
#include "esp_wifi.h"
#include "esp_wifi_mesh_priv.h"

LOG_MODULE_REGISTER(esp_wifi_mesh_netif, CONFIG_WIFI_LOG_LEVEL);

/*
 * The mesh transport caps the payload at MESH_MPS bytes, and each IP packet is
 * carried as an Ethernet frame (payload plus the Ethernet header). Cap the
 * interface MTU so a full-size packet still fits the mesh payload rather than
 * being dropped by the transport.
 */
#define MESH_NETIF_MTU           (MESH_MPS - sizeof(struct net_eth_hdr))
#define MESH_NETIF_RX_STACK      4096
#define MESH_NETIF_RX_TIMEOUT_MS 1000
#define MESH_ETH_FRAME_MAX       (MESH_NETIF_MTU + sizeof(struct net_eth_hdr))

struct mesh_netif_data {
	struct net_if *iface;
	uint8_t mac[6];
};

static struct mesh_netif_data mesh_netif;
static uint8_t rx_frame[MESH_ETH_FRAME_MAX];

/*
 * Outbound frames are queued and sent from a dedicated thread. esp_mesh_send()
 * is not reentrant and blocks until delivery, and the root forwards frames from
 * within the mesh receive path (the network stack forwards a masqueraded reply
 * straight to this interface). Sending inline there would call the mesh stack
 * reentrantly from its own task and corrupt it, so a queue decouples the two.
 */
struct mesh_tx_msg {
	uint16_t len;
	uint8_t frame[MESH_ETH_FRAME_MAX];
};

K_MSGQ_DEFINE(esp_wifi_mesh_tx_msgq, sizeof(struct mesh_tx_msg), 8, 4);

static void mesh_tx_frame(struct mesh_tx_msg *msg)
{
	mesh_data_t data = {
		.data = msg->frame,
		.size = msg->len,
		/*
		 * The protocol tags the direction: a frame from the root softAP
		 * down to a node carries MESH_PROTO_STA, a frame from a node
		 * station up to the root carries MESH_PROTO_AP. The receiver uses
		 * it to deliver the frame to the matching interface.
		 */
		.proto = esp_mesh_is_root() ? MESH_PROTO_STA : MESH_PROTO_AP,
		.tos = MESH_TOS_P2P,
	};

	if (esp_mesh_is_root()) {
		/*
		 * The Ethernet destination is the child's mesh address (the
		 * interface link address is the node's mesh MAC). A broadcast
		 * frame, such as an ARP request, is sent to every child in the
		 * routing table.
		 */
		static const uint8_t bcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
		mesh_addr_t to;

		if (memcmp(msg->frame, bcast, sizeof(bcast)) == 0) {
			/*
			 * The root routing table holds the whole sub-network, not
			 * just direct children, so size for the full tree.
			 */
			static mesh_addr_t route[CONFIG_WIFI_ESP32_MESH_MAX_LAYER *
							 CONFIG_WIFI_ESP32_MESH_MAX_CONNECTIONS +
						 1];
			int n = 0;

			esp_wifi_mesh_read_routing_table(route, sizeof(route), &n);

			for (int i = 0; i < n; i++) {
				if (memcmp(route[i].addr, mesh_netif.mac, sizeof(mesh_netif.mac)) ==
				    0) {
					continue;
				}
				if (esp_mesh_send(&route[i], &data, MESH_DATA_P2P, NULL, 0) !=
				    ESP_OK) {
					LOG_DBG("mesh downlink send failed");
				}
			}
		} else {
			memcpy(to.addr, msg->frame, sizeof(to.addr));
			if (esp_mesh_send(&to, &data, MESH_DATA_P2P, NULL, 0) != ESP_OK) {
				LOG_DBG("mesh downlink send failed");
			}
		}
	} else {
		/*
		 * Non-root: hand the frame up toward the root. The mesh routes a
		 * toDS frame to the root, which delivers it to its softAP-side
		 * interface for the network stack to masquerade and forward.
		 */
		if (esp_mesh_send(NULL, &data, MESH_DATA_TODS, NULL, 0) != ESP_OK) {
			LOG_DBG("mesh uplink send failed");
		}
	}
}

static void mesh_tx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static struct mesh_tx_msg msg;

	while (1) {
		if (k_msgq_get(&esp_wifi_mesh_tx_msgq, &msg, K_FOREVER) == 0) {
			mesh_tx_frame(&msg);
		}
	}
}

K_THREAD_DEFINE(esp_wifi_mesh_tx_id, 4096, mesh_tx_thread, NULL, NULL, NULL, 5, 0, 0);

/*
 * net_if TX: queue an outbound IP frame for the mesh transmit thread. The frame
 * is staged in a file-scope buffer rather than on the stack because it is close
 * to the interface MTU, which would overflow the small network TX thread stack.
 * The network stack serializes transmit calls per interface, so a single
 * staging buffer is safe here.
 */
static struct mesh_tx_msg mesh_tx_staging;

static int mesh_netif_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);

	size_t len = net_pkt_get_len(pkt);

	if (len < sizeof(struct net_eth_hdr) || len > sizeof(mesh_tx_staging.frame)) {
		return -EMSGSIZE;
	}
	if (net_pkt_read(pkt, mesh_tx_staging.frame, len) < 0) {
		return -EIO;
	}

	mesh_tx_staging.len = len;
	if (k_msgq_put(&esp_wifi_mesh_tx_msgq, &mesh_tx_staging, K_NO_WAIT) != 0) {
		return -ENOBUFS;
	}

	/* The Ethernet L2 unrefs the packet after a successful send. */
	return 0;
}

/*
 * Mesh RX: an inbound mesh frame becomes a net_pkt for the IP stack.
 *
 * TODO: this task and the raw esp_wifi_mesh_recv() path both read the single
 * mesh receive queue with no demux, so they are kept mutually exclusive (the
 * raw path is compiled out when MESH_IP is set). Folding both into one reader
 * that demultiplexes on the frame protocol tag (MESH_PROTO_STA/_AP here,
 * MESH_PROTO_BIN for the raw path) would let them coexist.
 */
static void mesh_netif_rx_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	mesh_addr_t from;
	int flag = 0;

	while (1) {
		mesh_data_t data = {
			.data = rx_frame,
			.size = sizeof(rx_frame),
		};

		if (esp_mesh_recv(&from, &data, MESH_NETIF_RX_TIMEOUT_MS, &flag, NULL, 0) !=
		    ESP_OK) {
			continue;
		}

		struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(mesh_netif.iface, data.size,
								   NET_AF_UNSPEC, 0, K_MSEC(100));

		if (pkt == NULL) {
			continue;
		}
		if (net_pkt_write(pkt, data.data, data.size) < 0) {
			net_pkt_unref(pkt);
			continue;
		}
		if (net_recv_data(mesh_netif.iface, pkt) < 0) {
			net_pkt_unref(pkt);
		}
	}
}

K_THREAD_DEFINE(esp_wifi_mesh_rx_id, MESH_NETIF_RX_STACK, mesh_netif_rx_task, NULL, NULL, NULL, 5,
		0, 0);

/*
 * Root toDS drain: once the root announces external reachability, the mesh
 * queues child packets bound for an external IP in a separate toDS queue.
 * Drain it and inject the frames into the network stack, which masquerades and
 * forwards them out the station just like any other received frame.
 */
static uint8_t tods_frame[MESH_ETH_FRAME_MAX];

static void mesh_tods_rx_task(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	mesh_addr_t from;
	mesh_addr_t to;
	int flag = 0;

	while (1) {
		mesh_data_t data = {
			.data = tods_frame,
			.size = sizeof(tods_frame),
		};

		if (!esp_mesh_is_root()) {
			k_sleep(K_MSEC(500));
			continue;
		}
		if (esp_mesh_recv_toDS(&from, &to, &data, MESH_NETIF_RX_TIMEOUT_MS, &flag, NULL,
				       0) != ESP_OK) {
			continue;
		}

		struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(mesh_netif.iface, data.size,
								   NET_AF_UNSPEC, 0, K_MSEC(100));

		if (pkt == NULL) {
			continue;
		}
		if (net_pkt_write(pkt, data.data, data.size) < 0) {
			net_pkt_unref(pkt);
			continue;
		}
		if (net_recv_data(mesh_netif.iface, pkt) < 0) {
			net_pkt_unref(pkt);
		}
	}
}

K_THREAD_DEFINE(esp_wifi_mesh_tods_id, MESH_NETIF_RX_STACK, mesh_tods_rx_task, NULL, NULL, NULL, 5,
		0, 0);

static void mesh_netif_iface_init(struct net_if *iface)
{
	mesh_netif.iface = iface;

	esp_wifi_get_mac(ESP_IF_WIFI_STA, mesh_netif.mac);
	net_if_set_link_addr(iface, mesh_netif.mac, sizeof(mesh_netif.mac), NET_LINK_ETHERNET);

	ethernet_init(iface);

	/* The mesh stack provides the link, so present a live carrier. */
	net_if_carrier_on(iface);
}

static const struct ethernet_api mesh_netif_api = {
	.iface_api.init = mesh_netif_iface_init,
	.send = mesh_netif_send,
};

/* A standalone Ethernet net_if that the Zephyr IP stack drives. */
NET_DEVICE_INIT(mesh_netif, "mesh_netif", NULL, NULL, &mesh_netif, NULL, CONFIG_ETH_INIT_PRIORITY,
		&mesh_netif_api, ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2), MESH_NETIF_MTU);

struct net_if *esp_wifi_mesh_netif_get(void)
{
	return mesh_netif.iface;
}
