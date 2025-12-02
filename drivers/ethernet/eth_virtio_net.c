/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/random/random.h>
#include "eth.h"

#define DT_DRV_COMPAT virtio_net
LOG_MODULE_REGISTER(virtio_net, CONFIG_ETHERNET_LOG_LEVEL);

enum _virtio_feature_bits {
	VIRTIO_NET_F_CSUM,
	VIRTIO_NET_F_GUEST_CSUM,
	VIRTIO_NET_F_CTRL_GUEST_OFFLOADS,
	VIRTIO_NET_F_MTU,
	VIRTIO_NET_F_MAC = 5,
	VIRTIO_NET_F_GUEST_TSO4 = 7,
	VIRTIO_NET_F_GUEST_TSO6,
	VIRTIO_NET_F_GUEST_ECN,
	VIRTIO_NET_F_GUEST_UFO,
	VIRTIO_NET_F_HOST_TSO4,
	VIRTIO_NET_F_HOST_TSO6,
	VIRTIO_NET_F_HOST_ECN,
	VIRTIO_NET_F_HOST_UFO,
	VIRTIO_NET_F_MRG_RXBUF,
	VIRTIO_NET_F_STATUS,
	VIRTIO_NET_F_CTRL_VQ,
	VIRTIO_NET_F_CTRL_RX,
	VIRTIO_NET_F_CTRL_VLAN,
	VIRTIO_NET_F_CTRL_RX_EXTRA,
	VIRTIO_NET_F_GUEST_ANNOUNCE,
	VIRTIO_NET_F_MQ,
	VIRTIO_NET_F_CTRL_MAC_ADDR,
	VIRTIO_NET_F_HASH_TUNNEL = 51,
	VIRTIO_NET_F_VQ_NOTF_COAL,
	VIRTIO_NET_F_NOTF_COAL,
	VIRTIO_NET_F_GUEST_USO4,
	VIRTIO_NET_F_GUEST_USO6,
	VIRTIO_NET_F_HOST_USO,
	VIRTIO_NET_F_HASH_REPORT,
	VIRTIO_NET_F_GUEST_HDRLEN = 59,
	VIRTIO_NET_F_RSS,
	VIRTIO_NET_F_RSC_EXT,
	VIRTIO_NET_F_STANDBY,
	VIRTIO_NET_F_SPEED_DUPLEX
};

struct _virtio_net_config {
	uint8_t mac[6];
	/* More fields exist if certain features are set by the device */
};

/* Header prepending every sent and received Ethernet frame */
struct _virtio_net_hdr {
	uint8_t flags;
	uint8_t gso_type;
	uint16_t hdr_len;
	uint16_t gso_size;
	uint16_t csum_start;
	uint16_t csum_offset;
	uint16_t num_buffers;
	/* There are three more fields if device has VIRTIO_NET_F_HASH_REPORT set */
};

enum _virtio_net_hdr_flags {
	VIRTIO_NET_HDR_F_NEEDS_CSUM = 1,
	VIRTIO_NET_HDR_F_DATA_VALID = 2,
	VIRTIO_NET_HDR_F_RSC_INFO = 4
};

enum _virtio_net_hdr_gso_types {
	VIRTIO_NET_HDR_GSO_NONE,
	VIRTIO_NET_HDR_GSO_TCPV4,
	VIRTIO_NET_HDR_GSO_UDP = 3,
	VIRTIO_NET_HDR_GSO_TCPV6,
	VIRTIO_NET_HDR_GSO_UDP_L4,
	VIRTIO_NET_HDR_GSO_ECN = 0x80
};

#define VIRTIO_NET_BUFLEN                                                                          \
	(NET_ETH_MTU + sizeof(struct net_eth_hdr) + sizeof(struct _virtio_net_hdr))
/* virtqueue pairs are numbered from 1 upwards */
/* convert pair number to virtqueue index */
#define VIRTQ_RX(n) ((n - 1) * 2)
#define VIRTQ_TX(n) (VIRTQ_RX(n) + 1)

struct virtnet_config {
	const struct device *vdev;
	bool random_mac;
	unsigned int inst;
};

/* Allows virtnet_rx_cb to know which virtqueue it was called by */
struct _rx_cb_data {
	struct virtnet_data *data;
	uint16_t buf_no;
};

struct virtnet_data {
	const struct device *dev;
	struct net_if *iface;
	const struct _virtio_net_config *virtio_devcfg;
	uint8_t mac[6];
	struct _rx_cb_data rx_cb_data[CONFIG_ETH_VIRTIO_NET_RX_BUFFERS];
	uint8_t txb[VIRTIO_NET_BUFLEN];
	uint8_t rxb[CONFIG_ETH_VIRTIO_NET_RX_BUFFERS][VIRTIO_NET_BUFLEN];
};

static uint16_t virtnet_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *)
{
	if (q_index % 2 == 0) { /* receiving virtqueue (even-numbered) */
		return CONFIG_ETH_VIRTIO_NET_RX_BUFFERS;
	} else {
		return 1;
	}
}

static enum ethernet_hw_caps virtnet_get_capabilities(const struct device *dev)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_LINK_1000BASE |
	       ETHERNET_LINK_2500BASE | ETHERNET_LINK_5000BASE;
}

static int virtnet_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct virtnet_config *config = dev->config;
	struct virtnet_data *data = dev->data;
	size_t len = net_pkt_get_len(pkt);

	if (net_pkt_read(pkt, data->txb + sizeof(struct _virtio_net_hdr), len)) {
		LOG_ERR("could not read contents of packet to be sent");
		return -EIO;
	}

	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_TX(1));
	struct virtq_buf vqbuf[] = {
		{.addr = data->txb, .len = sizeof(struct _virtio_net_hdr) + len}};

	if (virtq_add_buffer_chain(vq, vqbuf, 1, 1, NULL, NULL, K_FOREVER)) {
		LOG_ERR("could not send packet");
		return -EIO;
	}
	virtio_notify_virtqueue(config->vdev, VIRTQ_TX(1));
	return 0;
}

void virtnet_rx_cb(void *priv, uint32_t len)
{
	const struct _rx_cb_data *p = priv;
	struct virtnet_data *data = p->data;
	uint16_t buf_no = p->buf_no;
	const struct virtnet_config *config = data->dev->config;
	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_RX(1));

	len -= sizeof(struct _virtio_net_hdr);
	struct net_pkt *pkt =
		net_pkt_rx_alloc_with_buffer(data->iface, len, AF_UNSPEC, 0, K_FOREVER);

	if (pkt == NULL) {
		LOG_ERR("received packet, but could not pass it to the operating system");
	} else if (net_pkt_write(pkt, &(data->rxb[buf_no][sizeof(struct _virtio_net_hdr)]), len)) {
		LOG_ERR("could not copy entire received packet");
		net_pkt_unref(pkt);
	} else if (net_recv_data(data->iface, pkt)) {
		LOG_ERR("operating system failed to receive packet");
		net_pkt_unref(pkt);
	} else {
		/* Packet received correctly, no error */
	}
	struct virtq_buf vqbuf[] = {{.addr = &(data->rxb[buf_no]), .len = VIRTIO_NET_BUFLEN}};

	virtq_add_buffer_chain(vq, vqbuf, 1, 0, virtnet_rx_cb, priv, K_FOREVER);
	virtio_notify_virtqueue(config->vdev, VIRTQ_RX(1));
}

static void virtnet_if_init(struct net_if *iface)
{
	ethernet_init(iface);
	const struct device *dev = net_if_get_device(iface);
	struct virtnet_data *data = dev->data;
	const struct virtnet_config *config = dev->config;

	if (dev == NULL) {
		LOG_ERR("could not access device structure!");
		return;
	}
	data->iface = iface;
	net_if_set_link_addr(iface, data->mac, sizeof(data->virtio_devcfg->mac), NET_LINK_ETHERNET);
	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_RX(1));

	for (int i = 0; i < CONFIG_ETH_VIRTIO_NET_RX_BUFFERS; i++) {
		data->rx_cb_data[i].data = data;
		data->rx_cb_data[i].buf_no = i;

		struct virtq_buf vqbuf[] = {{.addr = &(data->rxb[i]), .len = VIRTIO_NET_BUFLEN}};

		virtq_add_buffer_chain(vq, vqbuf, 1, 0, virtnet_rx_cb, &(data->rx_cb_data[i]),
				       K_FOREVER);
		virtio_notify_virtqueue(config->vdev, VIRTQ_RX(1));
	}
	LOG_DBG("initialization finished");
}

static int virtnet_dev_init(const struct device *dev)
{
	const struct virtnet_config *config = dev->config;
	struct virtnet_data *data = dev->data;

	if (config->random_mac) {
		sys_rand_get(data->mac, sizeof(data->mac));
		/* make it a locally administered, unicast address */
		/* (2nd hex digit of 1st byte is 2) */
		data->mac[0] &= ~(BIT(0) | BIT(2) | BIT(3));
		data->mac[0] |= BIT(1);
	}

	data->virtio_devcfg = virtio_get_device_specific_config(config->vdev);
	if (data->virtio_devcfg == NULL) {
		LOG_ERR("could not get config struct");
	}
	if (virtio_commit_feature_bits(config->vdev)) {
		LOG_ERR("could not commit feature bits");
	}
	LOG_DBG("MAC address is %02x:%02x:%02x:%02x:%02x:%02x", data->mac[0], data->mac[1],
		data->mac[2], data->mac[3], data->mac[4], data->mac[5]);

	virtio_init_virtqueues(config->vdev, 2, virtnet_enum_queues_cb, NULL);
	virtio_finalize_init(config->vdev);

	return 0;
}

static struct ethernet_api virtnet_api = {
	.iface_api.init = virtnet_if_init,
	.get_capabilities = virtnet_get_capabilities,
	.send = virtnet_send,
};

#define VIRTIO_NET_DEFINE(inst)                                                                    \
	static struct virtnet_data virtnet_data_##inst = {                                         \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
		.mac = DT_INST_PROP_OR(inst, local_mac_address, {0}),                              \
	};                                                                                         \
	static const struct virtnet_config virtnet_config_##inst = {                               \
		.vdev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                       \
		.random_mac = DT_INST_PROP(inst, zephyr_random_mac_address),                       \
	};                                                                                         \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, virtnet_dev_init, NULL, &virtnet_data_##inst,          \
				      &virtnet_config_##inst, CONFIG_ETH_INIT_PRIORITY,            \
				      &virtnet_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(VIRTIO_NET_DEFINE);
