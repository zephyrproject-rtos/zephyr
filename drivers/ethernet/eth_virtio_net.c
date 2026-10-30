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
#include <ethernet/eth_stats.h>
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

#define VIRTIO_NET_CTRL_RX            0
#define VIRTIO_NET_CTRL_RX_PROMISC    0
#define VIRTIO_NET_CTRL_RX_ALLMULTI   1
#define VIRTIO_NET_CTRL_MAC           1
#define VIRTIO_NET_CTRL_MAC_TABLE_SET 0
#define VIRTIO_NET_CTRL_MAC_ADDR_SET  1

#if defined(CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER)
/* MAC address table of the VIRTIO_NET_CTRL_MAC_TABLE_SET command. The
 * command carries two of these, first the unicast and then the multicast
 * table. Sized by what the Ethernet L2 tracks for the interface.
 */
struct _virtio_net_ctrl_mac {
	uint32_t entries; /* little-endian */
	uint8_t macs[NET_ETH_MCAST_FILTER_COUNT][NET_ETH_ADDR_LEN];
} __packed;
#endif /* CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER */

/* Each receive buffer is a fragment of the receive buffer pool. With
 * mergeable receive buffers a frame is spread over as many of them as it
 * needs, the first one starting with the header, and all of them must be
 * available at once for the device to deliver a maximum size frame.
 */
#define VIRTNET_RX_BUF_SIZE CONFIG_NET_BUF_DATA_SIZE
BUILD_ASSERT(CONFIG_ETH_VIRTIO_NET_RX_BUFFERS * VIRTNET_RX_BUF_SIZE >=
		     NET_ETH_MAX_FRAME_SIZE + sizeof(struct _virtio_net_hdr),
	     "the receive buffers cannot hold a maximum size frame");

#define VIRTNET_RX_REFILL_STACK_SIZE 1024

/* Fragments of a maximum size frame, plus the separate link layer header
 * fragment of a packet forwarded from another interface
 */
#define VIRTNET_TX_MAX_FRAGS (DIV_ROUND_UP(NET_ETH_MAX_FRAME_SIZE, CONFIG_NET_BUF_DATA_SIZE) + 1)
/* Descriptors of one transmitted packet: the header and its fragments */
#define VIRTNET_TX_CHAIN_LEN (1 + VIRTNET_TX_MAX_FRAGS)

/* Grace period to wait for a free transmit slot or descriptor. The device
 * returns a packet as soon as it has processed it, so this is only reached
 * when it stalls. Beyond that the packet is dropped.
 */
#define VIRTNET_TX_AVAIL_WAIT K_MSEC(10)

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

/* A receive buffer handed to the device, or waiting for a fragment of the
 * receive buffer pool to become free
 */
struct virtnet_rx_slot {
	void *fifo_reserved;
	struct virtnet_data *data;
	struct net_buf *buf;
};

/* A packet handed to the device: its fragments are referenced by the
 * descriptor chain following the header, so nothing is copied
 */
struct virtnet_tx_slot {
	struct _virtio_net_hdr hdr;
	struct virtnet_data *data;
	struct net_pkt *pkt;
	struct virtq_buf bufs[VIRTNET_TX_CHAIN_LEN];
};

struct virtnet_data {
	const struct device *dev;
	struct net_if *iface;
	const struct _virtio_net_config *virtio_devcfg;
	uint8_t mac[6];
	struct k_mem_slab tx_slab;
	struct virtnet_tx_slot tx_slots[CONFIG_ETH_VIRTIO_NET_TX_BUFFERS];
	struct virtnet_rx_slot rx_slots[CONFIG_ETH_VIRTIO_NET_RX_BUFFERS];
	/* Slots in use, the device may offer fewer descriptors than slots */
	uint16_t rx_slot_count;
	/* Slots whose buffer could not be replaced from the interrupt */
	struct k_fifo rx_refill;
	struct k_thread rx_refill_thread;

	K_KERNEL_STACK_MEMBER(rx_refill_stack, VIRTNET_RX_REFILL_STACK_SIZE);

	/* Frame being assembled from consecutive receive buffers */
	struct net_pkt *rx_pkt;
	uint16_t rx_remaining;
	/* Control virtqueue command. The buffers must stay valid until the
	 * device used them, so there is one in-flight command, serialized by
	 * ctrl_lock and completed through ctrl_sem.
	 */
	struct k_mutex ctrl_lock;
	struct k_sem ctrl_sem;
	struct _virtio_net_ctrl_hdr ctrl_hdr;
#if defined(CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER)
	uint32_t ctrl_uni_entries;
	struct _virtio_net_ctrl_mac ctrl_multi;
#endif /* CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER */
	uint8_t ctrl_ack;
	uint8_t ctrl_onoff;
	/* VIRTIO_NET_F_CTRL_VQ and VIRTIO_NET_F_CTRL_RX were negotiated */
	bool has_ctrl_rx;
	/* VIRTIO_NET_F_CTRL_VQ and VIRTIO_NET_F_CTRL_MAC_ADDR were negotiated */
	bool has_mac_addr_set;
};

static uint16_t virtnet_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *priv)
{
	struct virtnet_data *data = priv;

	if (q_index == VIRTQ_CTRL) {
		/* Big enough for the descriptor chain of one command */
		return MIN(8, q_size_max);
	}

	if (q_index % 2 == 0) { /* receiving virtqueue (even-numbered) */
		/* one descriptor per receive buffer */
		data->rx_slot_count = MIN(CONFIG_ETH_VIRTIO_NET_RX_BUFFERS, q_size_max);
		return NHPOT(data->rx_slot_count);
	}

	/* transmitting virtqueue: room for the descriptor chains of all the
	 * packets that may be in flight
	 */
	return MIN(NHPOT(CONFIG_ETH_VIRTIO_NET_TX_BUFFERS * VIRTNET_TX_CHAIN_LEN), q_size_max);
}

static enum ethernet_hw_caps virtnet_get_capabilities(const struct device *dev,
						     struct net_if *iface __unused)
{
	const struct virtnet_data *data = dev->data;
	enum ethernet_hw_caps caps = ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE |
				     ETHERNET_LINK_1000BASE | ETHERNET_LINK_2500BASE |
				     ETHERNET_LINK_5000BASE;

	if (data->has_ctrl_rx) {
		if (IS_ENABLED(CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER)) {
			caps |= ETHERNET_HW_FILTERING;
		}

		if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			caps |= ETHERNET_PROMISC_MODE;
		}
	}

	return caps;
}

static void virtnet_ctrl_cb(void *priv, uint32_t len)
{
	struct virtnet_data *data = priv;

	ARG_UNUSED(len);

	k_sem_give(&data->ctrl_sem);
}

#if defined(CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER)
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
#endif /* CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER */

/* Set or clear one of the receive mode flags of the device */
static int virtnet_rx_mode_set(const struct device *dev, uint8_t cmd, bool on)
{
	const struct virtnet_config *config = dev->config;
	struct virtnet_data *data = dev->data;
	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_CTRL);
	int ret;

	if (vq == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&data->ctrl_lock, K_FOREVER);

	data->ctrl_hdr.class = VIRTIO_NET_CTRL_RX;
	data->ctrl_hdr.cmd = cmd;
	data->ctrl_onoff = on ? 1 : 0;
	data->ctrl_ack = VIRTIO_NET_ERR;

	struct virtq_buf bufs[] = {
		{.addr = &data->ctrl_hdr, .len = sizeof(data->ctrl_hdr)},
		{.addr = &data->ctrl_onoff, .len = sizeof(data->ctrl_onoff)},
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

/* A device may start out in promiscuous mode, as QEMU's does, so that
 * drivers without the receive filter commands receive everything. Leave
 * that mode so the MAC table takes effect, or if the table is not kept up
 * to date, so all multicast is received instead.
 */
static int virtnet_rx_mode_init(const struct device *dev)
{
	int ret;

	if (!IS_ENABLED(CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER)) {
		ret = virtnet_rx_mode_set(dev, VIRTIO_NET_CTRL_RX_ALLMULTI, true);
		if (ret != 0) {
			return ret;
		}
	}

	return virtnet_rx_mode_set(dev, VIRTIO_NET_CTRL_RX_PROMISC, false);
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

	switch (type) {
#if defined(CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER)
	case ETHERNET_CONFIG_TYPE_FILTER:
		struct net_eth_addr mac;

		if (!data->has_ctrl_rx) {
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
#endif /* CONFIG_ETH_VIRTIO_NET_MULTICAST_FILTER */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (!data->has_ctrl_rx) {
			return -ENOTSUP;
		}

		return virtnet_rx_mode_set(dev, VIRTIO_NET_CTRL_RX_PROMISC,
					   net_config->promisc_mode);
#endif /* CONFIG_NET_PROMISCUOUS_MODE */
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

static void virtnet_tx_cb(void *priv, uint32_t len __unused)
{
	struct virtnet_tx_slot *slot = priv;
	struct virtnet_data *data = slot->data;

	net_pkt_unref(slot->pkt);
	k_mem_slab_free(&data->tx_slab, slot);
}

static int virtnet_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct virtnet_config *config = dev->config;
	struct virtnet_data *data = dev->data;
	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_TX(1));
	struct virtnet_tx_slot *slot;
	uint16_t nbufs = 1;
	int ret;

	ret = k_mem_slab_alloc(&data->tx_slab, (void **)&slot, VIRTNET_TX_AVAIL_WAIT);
	if (ret != 0) {
		LOG_DBG("no free transmit slot");
		return -ENOMEM;
	}

	/* No offloads are negotiated, so the header carries nothing */
	memset(&slot->hdr, 0, sizeof(slot->hdr));
	slot->data = data;
	slot->pkt = pkt;
	slot->bufs[0].addr = &slot->hdr;
	slot->bufs[0].len = sizeof(slot->hdr);

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		if (frag->len == 0U) {
			continue;
		}

		if (nbufs == ARRAY_SIZE(slot->bufs)) {
			LOG_ERR("packet has too many fragments");
			k_mem_slab_free(&data->tx_slab, slot);
			return -ENOMEM;
		}

		slot->bufs[nbufs].addr = frag->data;
		slot->bufs[nbufs].len = frag->len;
		nbufs++;
	}

	LOG_DBG("pkt len/bufs=%zu/%u", net_pkt_get_len(pkt), nbufs);

	/* The fragments stay with the device until it returns the chain */
	net_pkt_ref(pkt);

	ret = virtq_add_buffer_chain(vq, slot->bufs, nbufs, nbufs, virtnet_tx_cb, slot,
				     VIRTNET_TX_AVAIL_WAIT);
	if (ret != 0) {
		LOG_DBG("no free transmit descriptors");
		net_pkt_unref(pkt);
		k_mem_slab_free(&data->tx_slab, slot);
		return -ENOMEM;
	}

	virtio_notify_virtqueue(config->vdev, VIRTQ_TX(1));

	return 0;
}

static void virtnet_rx_cb(void *priv, uint32_t len);

/* Hand a fragment of the receive buffer pool to the device */
static int virtnet_rx_slot_fill(struct virtnet_rx_slot *slot, k_timeout_t timeout)
{
	struct virtnet_data *data = slot->data;
	const struct virtnet_config *config = data->dev->config;
	struct virtq *vq = virtio_get_virtqueue(config->vdev, VIRTQ_RX(1));
	int ret;

	slot->buf = net_pkt_get_reserve_rx_data(VIRTNET_RX_BUF_SIZE, timeout);
	if (slot->buf == NULL) {
		return -ENOMEM;
	}

	struct virtq_buf vqbuf[] = {{.addr = slot->buf->data, .len = slot->buf->size}};

	/* The virtqueue has a descriptor for every slot */
	ret = virtq_add_buffer_chain(vq, vqbuf, 1, 0, virtnet_rx_cb, slot, K_NO_WAIT);
	if (ret != 0) {
		net_buf_unref(slot->buf);
		slot->buf = NULL;
		return ret;
	}

	virtio_notify_virtqueue(config->vdev, VIRTQ_RX(1));

	return 0;
}

static void virtnet_rx_refill_thread(void *arg1, void *arg2 __unused, void *arg3 __unused)
{
	struct virtnet_data *data = arg1;

	while (true) {
		struct virtnet_rx_slot *slot = k_fifo_get(&data->rx_refill, K_FOREVER);

		if (virtnet_rx_slot_fill(slot, K_FOREVER) != 0) {
			LOG_ERR("could not hand a receive buffer to the device");
		}
	}
}

/* The first buffer of a frame starts with the header, which tells how many
 * buffers the frame is spread over
 */
static int virtnet_rx_frame_start(struct virtnet_data *data, struct net_buf *buf)
{
	const struct _virtio_net_hdr *hdr = (const struct _virtio_net_hdr *)buf->data;
	uint16_t num_buffers;

	if (buf->len < sizeof(*hdr)) {
		return -EINVAL;
	}

	num_buffers = sys_le16_to_cpu(hdr->num_buffers);
	if (num_buffers == 0U) {
		return -EINVAL;
	}

	data->rx_remaining = num_buffers;

	/* Without a packet the buffers of the frame are dropped as they come */
	data->rx_pkt = net_pkt_rx_alloc_on_iface(data->iface, K_NO_WAIT);
	if (data->rx_pkt == NULL) {
		eth_stats_update_errors_rx(data->iface);
	}

	net_buf_pull(buf, sizeof(*hdr));

	return 0;
}

static void virtnet_rx_cb(void *priv, uint32_t len)
{
	struct virtnet_rx_slot *slot = priv;
	struct virtnet_data *data = slot->data;
	struct net_buf *buf = slot->buf;

	slot->buf = NULL;
	buf->len = MIN(len, buf->size);

	if (data->rx_remaining == 0U && virtnet_rx_frame_start(data, buf) != 0) {
		LOG_ERR("received buffer does not start a frame");
		eth_stats_update_errors_rx(data->iface);
		net_buf_unref(buf);
	} else {
		data->rx_remaining--;

		if (data->rx_pkt == NULL) {
			net_buf_unref(buf);
		} else {
			net_pkt_frag_add(data->rx_pkt, buf);
		}

		if (data->rx_remaining == 0U && data->rx_pkt != NULL) {
			LOG_DBG("pkt len=%zu", net_pkt_get_len(data->rx_pkt));

			if (net_recv_data(data->iface, data->rx_pkt) < 0) {
				eth_stats_update_errors_rx(data->iface);
				net_pkt_unref(data->rx_pkt);
			}

			data->rx_pkt = NULL;
		}
	}

	/* Replace the buffer right away if the pool allows, otherwise once
	 * the stack has released one
	 */
	if (virtnet_rx_slot_fill(slot, K_NO_WAIT) != 0) {
		LOG_DBG("receive buffer pool empty");
		k_fifo_put(&data->rx_refill, slot);
	}
}

static void virtnet_if_init(struct net_if *iface)
{
	ethernet_init(iface);
	const struct device *dev = net_if_get_device(iface);
	struct virtnet_data *data = dev->data;

	data->iface = iface;
	net_if_set_link_addr(iface, data->mac, sizeof(data->virtio_devcfg->mac), NET_LINK_ETHERNET);

	for (uint16_t i = 0; i < data->rx_slot_count; i++) {
		struct virtnet_rx_slot *slot = &data->rx_slots[i];

		slot->data = data;

		if (virtnet_rx_slot_fill(slot, K_NO_WAIT) != 0) {
			LOG_ERR("receive buffer pool too small for %u receive buffers",
				data->rx_slot_count);
			k_fifo_put(&data->rx_refill, slot);
		}
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
	k_fifo_init(&data->rx_refill);

	ret = k_mem_slab_init(&data->tx_slab, data->tx_slots, sizeof(data->tx_slots[0]),
			      ARRAY_SIZE(data->tx_slots));
	if (ret != 0) {
		return ret;
	}

	data->virtio_devcfg = virtio_get_device_specific_config(config->vdev);
	if (data->virtio_devcfg == NULL) {
		LOG_ERR("could not get config struct");
	}

	/* A frame is received into as many buffers as it needs, each one a
	 * fragment of the receive buffer pool
	 */
	if (!virtio_read_device_feature_bit(config->vdev, VIRTIO_NET_F_MRG_RXBUF) ||
	    virtio_write_driver_feature_bit(config->vdev, VIRTIO_NET_F_MRG_RXBUF, true) != 0) {
		LOG_ERR("device does not support mergeable receive buffers");
		return -ENOTSUP;
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

	/* MAC address filtering and the receive modes need the receive filter
	 * commands
	 */
	if (has_ctrl_vq && virtio_read_device_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_RX)) {
		if (virtio_write_driver_feature_bit(config->vdev, VIRTIO_NET_F_CTRL_RX, true)) {
			LOG_WRN("could not enable receive filter feature bit");
		} else {
			data->has_ctrl_rx = true;
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

	virtio_init_virtqueues(config->vdev, has_ctrl_vq ? 3 : 2, virtnet_enum_queues_cb, data);
	virtio_finalize_init(config->vdev);

	if (data->has_ctrl_rx && virtnet_rx_mode_init(dev) != 0) {
		LOG_WRN("could not leave promiscuous mode");
	}

	/* The driver chose its own MAC address, tell the device about it */
	if (data->has_mac_addr_set && ret == 0 && virtnet_mac_addr_set(dev) != 0) {
		LOG_WRN("could not tell the device the MAC address");
	}

	k_thread_create(&data->rx_refill_thread, data->rx_refill_stack,
			K_KERNEL_STACK_SIZEOF(data->rx_refill_stack), virtnet_rx_refill_thread,
			data, NULL, NULL, CONFIG_ETH_VIRTIO_NET_RX_REFILL_THREAD_PRIORITY,
			K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&data->rx_refill_thread, "virtnet_rx_refill");

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
