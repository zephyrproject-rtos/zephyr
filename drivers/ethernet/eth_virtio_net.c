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
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
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

/* Control virtqueue command header, followed by command-specific data and a
 * device-written ack byte
 */
struct _virtio_net_ctrl_hdr {
	uint8_t class;
	uint8_t cmd;
} __packed;

#define VIRTIO_NET_OK  0
#define VIRTIO_NET_ERR 1

#define VIRTIO_NET_CTRL_MAC           1
#define VIRTIO_NET_CTRL_MAC_TABLE_SET 0
#define VIRTIO_NET_CTRL_MAC_ADDR_SET  1

/* MAC address table of the VIRTIO_NET_CTRL_MAC_TABLE_SET command. The
 * command carries two of these, first the unicast and then the multicast
 * table. Sized by what the Ethernet L2 tracks for the interface.
 */
struct _virtio_net_ctrl_mac {
	uint32_t entries; /* little-endian */
	uint8_t macs[NET_ETH_MCAST_FILTER_COUNT][NET_ETH_ADDR_LEN];
} __packed;

#define VIRTIO_NET_BUFLEN                                                                          \
	(NET_ETH_MTU + sizeof(struct net_eth_hdr) + sizeof(struct _virtio_net_hdr))
/* virtqueue pairs are numbered from 1 upwards */
/* convert pair number to virtqueue index */
#define VIRTQ_RX(n) ((n - 1) * 2)
#define VIRTQ_TX(n) (VIRTQ_RX(n) + 1)
/* the control virtqueue comes after all the pairs, this driver uses one */
#define VIRTQ_CTRL  (2)

struct virtnet_config {
	const struct device *vdev;
	struct net_eth_mac_config mcfg;
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
	/* Control virtqueue command. The buffers must stay valid until the
	 * device used them, so there is one in-flight command, serialized by
	 * ctrl_lock and completed through ctrl_sem.
	 */
	struct k_mutex ctrl_lock;
	struct k_sem ctrl_sem;
	struct _virtio_net_ctrl_hdr ctrl_hdr;
	uint32_t ctrl_uni_entries;
	struct _virtio_net_ctrl_mac ctrl_multi;
	uint8_t ctrl_ack;
	/* VIRTIO_NET_F_CTRL_VQ and VIRTIO_NET_F_CTRL_RX were negotiated */
	bool has_mac_filter;
	/* VIRTIO_NET_F_CTRL_VQ and VIRTIO_NET_F_CTRL_MAC_ADDR were negotiated */
	bool has_mac_addr_set;
};

static uint16_t virtnet_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *priv)
{
	ARG_UNUSED(priv);

	if (q_index == VIRTQ_CTRL) {
		/* Big enough for the descriptor chain of one command */
		return MIN(8, q_size_max);
	}

	if (q_index % 2 == 0) { /* receiving virtqueue (even-numbered) */
		return CONFIG_ETH_VIRTIO_NET_RX_BUFFERS;
	} else {
		return 1;
	}
}

static enum ethernet_hw_caps virtnet_get_capabilities(const struct device *dev,
						     struct net_if *iface __unused)
{
	const struct virtnet_data *data = dev->data;
	enum ethernet_hw_caps caps = ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE |
				     ETHERNET_LINK_1000BASE | ETHERNET_LINK_2500BASE |
				     ETHERNET_LINK_5000BASE;

	if (data->has_mac_filter) {
		caps |= ETHERNET_HW_FILTERING;
	}

	return caps;
}

static void virtnet_ctrl_cb(void *priv, uint32_t len)
{
	struct virtnet_data *data = priv;

	ARG_UNUSED(len);

	k_sem_give(&data->ctrl_sem);
}

static void virtnet_mcast_collect_cb(struct net_if *iface,
				     const struct net_eth_mcast_addr *addr,
				     void *user_data)
{
	struct _virtio_net_ctrl_mac *table = user_data;
	uint32_t entries = sys_le32_to_cpu(table->entries);

	ARG_UNUSED(iface);

	if (entries >= ARRAY_SIZE(table->macs)) {
		return;
	}

	memcpy(table->macs[entries], addr->addr.addr, NET_ETH_ADDR_LEN);
	table->entries = sys_cpu_to_le32(entries + 1);
}

/* Give the device the whole multicast MAC table with the addresses the
 * Ethernet L2 tracks for the interface. The unicast table is left empty,
 * the device receives its own MAC address and broadcasts anyway.
 */
static int virtnet_mac_table_set(const struct device *dev, struct net_if *iface)
{
	const struct virtnet_config *config = dev->config;
	struct virtnet_data *data = dev->data;
	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_CTRL);
	int ret;

	if (vq == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&data->ctrl_lock, K_FOREVER);

	data->ctrl_hdr.class = VIRTIO_NET_CTRL_MAC;
	data->ctrl_hdr.cmd = VIRTIO_NET_CTRL_MAC_TABLE_SET;
	data->ctrl_uni_entries = sys_cpu_to_le32(0);
	data->ctrl_multi.entries = sys_cpu_to_le32(0);
	data->ctrl_ack = VIRTIO_NET_ERR;

	net_eth_mcast_addr_foreach(iface, virtnet_mcast_collect_cb, &data->ctrl_multi);

	struct virtq_buf bufs[] = {
		{.addr = &data->ctrl_hdr, .len = sizeof(data->ctrl_hdr)},
		{.addr = &data->ctrl_uni_entries, .len = sizeof(data->ctrl_uni_entries)},
		{.addr = &data->ctrl_multi,
		 .len = sizeof(data->ctrl_multi.entries) +
			sys_le32_to_cpu(data->ctrl_multi.entries) * NET_ETH_ADDR_LEN},
		{.addr = &data->ctrl_ack, .len = sizeof(data->ctrl_ack)},
	};

	/* Everything but the trailing ack byte is device-readable */
	ret = virtq_add_buffer_chain(vq, bufs, ARRAY_SIZE(bufs), ARRAY_SIZE(bufs) - 1,
				     virtnet_ctrl_cb, data, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("could not send control command");
		k_mutex_unlock(&data->ctrl_lock);
		return ret;
	}

	virtio_notify_virtqueue(config->vdev, VIRTQ_CTRL);
	k_sem_take(&data->ctrl_sem, K_FOREVER);

	ret = data->ctrl_ack == VIRTIO_NET_OK ? 0 : -EIO;

	k_mutex_unlock(&data->ctrl_lock);

	return ret;
}

/* Tell the device the MAC address the driver chose, so the device does not
 * pass through unicast traffic meant for other addresses
 */
static int virtnet_mac_addr_set(const struct device *dev)
{
	const struct virtnet_config *config = dev->config;
	struct virtnet_data *data = dev->data;
	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_CTRL);
	int ret;

	if (vq == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&data->ctrl_lock, K_FOREVER);

	data->ctrl_hdr.class = VIRTIO_NET_CTRL_MAC;
	data->ctrl_hdr.cmd = VIRTIO_NET_CTRL_MAC_ADDR_SET;
	data->ctrl_ack = VIRTIO_NET_ERR;

	struct virtq_buf bufs[] = {
		{.addr = &data->ctrl_hdr, .len = sizeof(data->ctrl_hdr)},
		{.addr = data->mac, .len = sizeof(data->mac)},
		{.addr = &data->ctrl_ack, .len = sizeof(data->ctrl_ack)},
	};

	/* Everything but the trailing ack byte is device-readable */
	ret = virtq_add_buffer_chain(vq, bufs, ARRAY_SIZE(bufs), ARRAY_SIZE(bufs) - 1,
				     virtnet_ctrl_cb, data, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("could not send control command");
		k_mutex_unlock(&data->ctrl_lock);
		return ret;
	}

	virtio_notify_virtqueue(config->vdev, VIRTQ_CTRL);
	k_sem_take(&data->ctrl_sem, K_FOREVER);

	ret = data->ctrl_ack == VIRTIO_NET_OK ? 0 : -EIO;

	k_mutex_unlock(&data->ctrl_lock);

	return ret;
}

static int virtnet_set_config(const struct device *dev, struct net_if *iface,
			      enum ethernet_config_type type,
			      const struct ethernet_config *net_config)
{
	struct virtnet_data *data = dev->data;
	struct net_eth_addr mac;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_FILTER:
		if (!data->has_mac_filter) {
			return -ENOTSUP;
		}

		/* The device filters multicast destination addresses only */
		mac = net_config->filter.mac_address;
		if (net_config->filter.type != ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS ||
		    !net_eth_is_addr_multicast(&mac)) {
			return -ENOTSUP;
		}

		/* The requested address is already part of the addresses the
		 * L2 tracks, so the whole table is simply programmed from
		 * them.
		 */
		return virtnet_mac_table_set(dev, iface);
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		/* Without the command the device would keep filtering with
		 * the old address
		 */
		if (!data->has_mac_addr_set) {
			return -ENOTSUP;
		}

		memcpy(data->mac, net_config->mac_address.addr, sizeof(data->mac));

		/* The caller updates the interface link address on success */
		return virtnet_mac_addr_set(dev);
	default:
		return -ENOTSUP;
	}
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
		net_pkt_rx_alloc_with_buffer(data->iface, len, NET_AF_UNSPEC, 0, K_FOREVER);

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
	bool has_devcfg_mac = false;
	bool has_ctrl_vq = false;
	int ret;

	k_mutex_init(&data->ctrl_lock);
	k_sem_init(&data->ctrl_sem, 0, 1);

	data->virtio_devcfg = virtio_get_device_specific_config(config->vdev);
	if (data->virtio_devcfg == NULL) {
		LOG_ERR("could not get config struct");
	}

	ret = net_eth_mac_load(&config->mcfg, data->mac);

	/* Without an explicit devicetree MAC configuration, use the address
	 * the device provides in the config space
	 */
	if (ret == -ENODATA && virtio_read_device_feature_bit(config->vdev, VIRTIO_NET_F_MAC)) {
		if (virtio_write_driver_feature_bit(config->vdev, VIRTIO_NET_F_MAC, true)) {
			LOG_WRN("could not enable device MAC address feature bit");
		} else {
			has_devcfg_mac = data->virtio_devcfg != NULL;
		}
	}

	/* The control virtqueue carries both the receive filter commands and
	 * the MAC address setting command
	 */
	if (virtio_read_device_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_VQ) &&
	    (virtio_read_device_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_RX) ||
	     virtio_read_device_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_MAC_ADDR))) {
		if (virtio_write_driver_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_VQ, true)) {
			LOG_WRN("could not enable control virtqueue feature bit");
		} else {
			has_ctrl_vq = true;
		}
	}

	/* MAC address filtering needs the receive filter commands */
	if (has_ctrl_vq && virtio_read_device_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_RX)) {
		if (virtio_write_driver_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_RX, true)) {
			LOG_WRN("could not enable MAC filtering feature bit");
		} else {
			data->has_mac_filter = true;
		}
	}

	/* Telling the device the driver's MAC address needs the MAC address
	 * setting command
	 */
	if (has_ctrl_vq &&
	    virtio_read_device_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_MAC_ADDR)) {
		if (virtio_write_driver_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_MAC_ADDR,
						    true)) {
			LOG_WRN("could not enable MAC address setting feature bit");
		} else {
			data->has_mac_addr_set = true;
		}
	}

	if (virtio_commit_feature_bits(config->vdev)) {
		LOG_ERR("could not commit feature bits");
	}

	if (has_devcfg_mac) {
		memcpy(data->mac, data->virtio_devcfg->mac, sizeof(data->mac));
	}

	LOG_DBG("MAC address is %02x:%02x:%02x:%02x:%02x:%02x", data->mac[0], data->mac[1],
		data->mac[2], data->mac[3], data->mac[4], data->mac[5]);

	virtio_init_virtqueues(config->vdev, has_ctrl_vq ? 3 : 2, virtnet_enum_queues_cb, NULL);
	virtio_finalize_init(config->vdev);

	/* The driver chose its own MAC address, tell the device about it */
	if (data->has_mac_addr_set && ret == 0 && virtnet_mac_addr_set(dev) != 0) {
		LOG_WRN("could not tell the device the MAC address");
	}

	return 0;
}

static struct ethernet_api virtnet_api = {
	.iface_api.init = virtnet_if_init,
	.get_capabilities = virtnet_get_capabilities,
	.set_config = virtnet_set_config,
	.send = virtnet_send,
};

#define VIRTIO_NET_DEFINE(inst)                                                                    \
	static struct virtnet_data virtnet_data_##inst = {                                         \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
	static const struct virtnet_config virtnet_config_##inst = {                               \
		.vdev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                       \
		.mcfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                                     \
	};                                                                                         \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, virtnet_dev_init, NULL, &virtnet_data_##inst,          \
				      &virtnet_config_##inst, CONFIG_ETH_INIT_PRIORITY,            \
				      &virtnet_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(VIRTIO_NET_DEFINE);
