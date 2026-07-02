/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_genet

#include "eth_bcmgenet_priv.h"

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bcmgenet, CONFIG_ETHERNET_LOG_LEVEL);

struct bcmgenet_data {
	DEVICE_MMIO_RAM;
	struct net_if *iface;
	struct net_pkt *rx_pkt;
	struct net_buf *rx_frags[GENET_NB_RX_DESCS];
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	uint16_t rx_idx;
	uint16_t tx_idx;
	uint16_t cidx;
	uint16_t pidx;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_BCMGENET_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
};

struct bcmgenet_config {
	DEVICE_MMIO_ROM;
	const struct device *phy_dev;
	struct net_eth_mac_config mac_cfg;
};

static inline uint32_t bcmgenet_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void bcmgenet_write(const struct device *dev, uint32_t reg, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_GET(dev) + reg);
}

static inline void bcmgenet_set_bits(const struct device *dev, uint32_t reg, uint32_t bits)
{
	sys_set_bits(DEVICE_MMIO_GET(dev) + reg, bits);
}

static inline void bcmgenet_clear_bits(const struct device *dev, uint32_t reg, uint32_t bits)
{
	sys_clear_bits(DEVICE_MMIO_GET(dev) + reg, bits);
}

static void bcmgenet_flush(const struct device *dev)
{
	bcmgenet_set_bits(dev, GENET_RBUF_FLUSH_CTRL, BIT(1));
	k_busy_wait(10);
	bcmgenet_clear_bits(dev, GENET_RBUF_FLUSH_CTRL, BIT(1));
	k_busy_wait(10);

	bcmgenet_set_bits(dev, GENET_MAC_TX_FLUSH, BIT(0));
	k_busy_wait(10);
	bcmgenet_clear_bits(dev, GENET_MAC_TX_FLUSH, BIT(0));
	k_busy_wait(10);
}

static void bcmgenet_reset(const struct device *dev)
{
	uint32_t value;

	value = GENET_MAC_SW_RST | GENET_MAC_LOOP_EN;
	bcmgenet_set_bits(dev, GENET_MAC_CMD, value);
	k_busy_wait(2);
	bcmgenet_clear_bits(dev, GENET_MAC_CMD, value);

	value = GENET_MAC_MIB_RST_RX | GENET_MAC_MIB_RST_TX | GENET_MAC_MIB_RST_RUNT;
	bcmgenet_write(dev, GENET_MAC_MIB_CTRL, value);
	k_busy_wait(2);
	bcmgenet_write(dev, GENET_MAC_MIB_CTRL, 0);
}

static void bcmgenet_ctrl(const struct device *dev, bool enable)
{
	if (enable) {
		bcmgenet_set_bits(dev, GENET_MAC_CMD, GENET_MAC_TX_EN | GENET_MAC_RX_EN);
	} else {
		bcmgenet_clear_bits(dev, GENET_MAC_CMD, GENET_MAC_TX_EN | GENET_MAC_RX_EN);
	}
}

static void bcmgenet_dma_ctrl(const struct device *dev, bool enable)
{
	uint32_t dma_ctrl;

	if (enable) {
		dma_ctrl = GENET_DMA_RING_BUF_EN(GENET_QUEUES) | GENET_DMA_EN;
		bcmgenet_set_bits(dev, GENET_TX_DMA_CTRL, dma_ctrl);
		bcmgenet_set_bits(dev, GENET_RX_DMA_CTRL, dma_ctrl);
	} else {
		bcmgenet_clear_bits(dev, GENET_TX_DMA_CTRL, GENET_DMA_EN);
		bcmgenet_clear_bits(dev, GENET_RX_DMA_CTRL, GENET_DMA_EN);
	}
}

static int bcmgenet_rx_refill(const struct device *dev, uint16_t index)
{
	struct bcmgenet_data *data = dev->data;
	struct net_buf *frag;
	uint32_t desc;

	frag = net_pkt_get_reserve_rx_data(GENET_BUF_SIZE, K_NO_WAIT);
	if (frag == NULL) {
		LOG_ERR("Failed to reserve data net buffers");
		data->rx_frags[index] = NULL;
		return -ENOBUFS;
	}

	data->rx_frags[index] = frag;
	desc = GENET_DMA_DESC_OFF_GET(index, GENET_RX_OFF);

	bcmgenet_write(dev, desc + GENET_DMA_DESC_ADDR_LOW, GENET_ADDR_LOW(frag->data));
	bcmgenet_write(dev, desc + GENET_DMA_DESC_ADDR_HIGH, GENET_ADDR_HIGH(frag->data));
	bcmgenet_write(dev, desc + GENET_DMA_DESC_STATUS,
		       GENET_DMA_RING_BUF_LEN_SET(GENET_BUF_SIZE) | GENET_DMA_OWN);

	return 0;
}

static void bcmgenet_rx_drain(const struct device *dev, uint16_t count)
{
	struct bcmgenet_data *data = dev->data;

	for (uint16_t i = 0; i < count; i++) {
		if (data->rx_frags[i] != NULL) {
			net_buf_unref(data->rx_frags[i]);
			data->rx_frags[i] = NULL;
		}
	}
}

static int bcmgenet_rx_ring_init(const struct device *dev)
{
	struct bcmgenet_data *data = dev->data;
	int ret;

	bcmgenet_write(dev, GENET_RX_DMA_SCB_BURST_SIZE, GENET_DMA_MAX_BURST_LEN);
	bcmgenet_write(dev, GENET_RX_DMA_START_ADDR, 0);
	bcmgenet_write(dev, GENET_RX_DMA_READ_PTR, 0);
	bcmgenet_write(dev, GENET_RX_DMA_WRITE_PTR, 0);
	bcmgenet_write(dev, GENET_RX_DMA_END_ADDR, GENET_DMA_DESC_SIZE_GET(GENET_NB_RX_DESCS));

	data->rx_idx = 0;
	bcmgenet_write(dev, GENET_RX_DMA_CIDX, bcmgenet_read(dev, GENET_RX_DMA_PIDX));

	bcmgenet_write(dev, GENET_RX_DMA_RING_BUF_SIZE,
		       GENET_DMA_RING_SIZE_SET(GENET_NB_RX_DESCS) | GENET_BUF_SIZE);
	bcmgenet_write(dev, GENET_RX_DMA_XOR_THRESH, GENET_DMA_FC_THRESH_VALUE);
	bcmgenet_write(dev, GENET_RX_DMA_RING_CFG, GENET_QUEUES_CFG);

	for (uint16_t i = 0; i < GENET_NB_RX_DESCS; i++) {
		ret = bcmgenet_rx_refill(dev, i);
		if (ret < 0) {
			bcmgenet_rx_drain(dev, i);
			return ret;
		}
	}

	return 0;
}

static int bcmgenet_rx(const struct device *dev)
{
	struct bcmgenet_data *data = dev->data;
	struct net_buf *frag;
	uint32_t desc_base, dma_flags;
	uint16_t pidx;
	int ret = 0;

	if (!net_if_flag_is_set(data->iface, NET_IF_UP)) {
		return -ENETDOWN;
	}

	pidx = (uint16_t)bcmgenet_read(dev, GENET_RX_DMA_PIDX) & UINT16_MAX;
	if (pidx == data->cidx) {
		return -EAGAIN;
	}

	frag = data->rx_frags[data->rx_idx];
	if (frag == NULL) {
		ret = bcmgenet_rx_refill(dev, data->rx_idx);
		if (ret < 0) {
			return ret;
		}
		return -EAGAIN;
	}

	desc_base = GENET_DMA_DESC_OFF_GET(data->rx_idx, GENET_RX_OFF);
	dma_flags = bcmgenet_read(dev, desc_base + GENET_DMA_DESC_STATUS);

	frag->len = GENET_DMA_RING_BUF_LEN_GET(dma_flags);
	sys_cache_data_invd_range(frag->data, frag->len);

	dma_flags &= UINT16_MAX;
	LOG_DBG("SOP: %d, EOP: %d", !!(dma_flags & GENET_DMA_SOP), !!(dma_flags & GENET_DMA_EOP));

	if (!(dma_flags & GENET_DMA_SOP) && data->rx_pkt == NULL) {
		LOG_WRN("Stale Rx descriptor at index: %u (flags=0x%x len=%u), skipping",
			data->rx_idx, dma_flags, frag->len);
		goto done;
	}

	if (dma_flags & GENET_DMA_SOP) {
		/* Starting a new frame - drop any stale one that never saw EOP */
		if (data->rx_pkt != NULL) {
			net_pkt_unref(data->rx_pkt);
			data->rx_pkt = NULL;
		}

		data->rx_pkt = net_pkt_rx_alloc_on_iface(data->iface, K_NO_WAIT);
		if (data->rx_pkt == NULL) {
			LOG_ERR("Failed to allocate pkt");
			ret = -ENOMEM;
			goto done;
		}
	}

	LOG_DBG("Rx fragment length: %d", frag->len);
	LOG_HEXDUMP_DBG(frag->data, frag->len, "Rx fragment");

	net_pkt_frag_add(data->rx_pkt, frag);
	if (dma_flags & GENET_DMA_EOP) {
		ret = net_recv_data(data->iface, data->rx_pkt);
		if (ret < 0) {
			LOG_ERR("Failed to enqueue frame to RX queue");
			net_pkt_unref(data->rx_pkt);
		}
		data->rx_pkt = NULL;
	}

done:
	ret = bcmgenet_rx_refill(dev, data->rx_idx);
	if (ret < 0) {
		bcmgenet_write(dev, desc_base + GENET_DMA_DESC_STATUS, 0);
		return ret;
	}

	data->cidx++;
	bcmgenet_write(dev, GENET_RX_DMA_CIDX, data->cidx);

	data->rx_idx++;
	if (data->rx_idx >= GENET_NB_RX_DESCS) {
		data->rx_idx = 0;
	}

	LOG_DBG("Rx index: %u, pidx: %u, cidx: %u", data->rx_idx, pidx, data->cidx);

	return ret;
}

static void bcmgenet_rx_thread(void *arg1, void *arg2 __unused, void *arg3 __unused)
{
	const struct device *dev = (struct device *)arg1;

	while (true) {
		while (bcmgenet_rx(dev) == 0) {
		/* drain all pending descriptors before sleeping */
		}
		k_usleep(CONFIG_ETH_BCMGENET_RX_POLLING_INTERVAL_US);
	}
}

static void bcmgenet_tx_ring_init(const struct device *dev)
{
	struct bcmgenet_data *data = dev->data;

	bcmgenet_write(dev, GENET_TX_DMA_SCB_BURST_SIZE, GENET_DMA_MAX_BURST_LEN);
	bcmgenet_write(dev, GENET_TX_DMA_START_ADDR, 0);
	bcmgenet_write(dev, GENET_TX_DMA_READ_PTR, 0);
	bcmgenet_write(dev, GENET_TX_DMA_WRITE_PTR, 0);
	bcmgenet_write(dev, GENET_TX_DMA_END_ADDR, GENET_DMA_DESC_SIZE_GET(GENET_NB_TX_DESCS));

	data->tx_idx = 0;
	bcmgenet_write(dev, GENET_TX_DMA_PIDX, bcmgenet_read(dev, GENET_TX_DMA_CIDX));

	bcmgenet_write(dev, GENET_TX_DMA_MBUF_DONE_THRESH, BIT(0));
	bcmgenet_write(dev, GENET_TX_DMA_FLOW_PERIOD, 0);
	bcmgenet_write(dev, GENET_TX_DMA_RING_BUF_SIZE,
		       GENET_DMA_RING_SIZE_SET(GENET_NB_TX_DESCS) | GENET_BUF_SIZE);

	bcmgenet_write(dev, GENET_TX_DMA_RING_CFG, GENET_QUEUES_CFG);
}

static int bcmgenet_send(const struct device *dev, struct net_pkt *pkt)
{
	struct bcmgenet_data *data = dev->data;
	uint32_t desc, len_ack;
	uint16_t nb_frags = 0;

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		sys_cache_data_flush_range(frag->data, frag->len);

		len_ack = GENET_DMA_RING_BUF_LEN_SET(frag->len) | GENET_DMA_TX_QTAG_SET(0x3F) |
			GENET_DMA_TX_APPEND_CRC;

		if (nb_frags == 0) {
			len_ack |= GENET_DMA_SOP;
		}

		if (!frag->frags) {
			len_ack |= GENET_DMA_EOP;
		}

		desc = GENET_DMA_DESC_OFF_GET(data->tx_idx, GENET_TX_OFF);
		bcmgenet_write(dev, desc + GENET_DMA_DESC_ADDR_LOW, GENET_ADDR_LOW(frag->data));
		bcmgenet_write(dev, desc + GENET_DMA_DESC_ADDR_HIGH, GENET_ADDR_HIGH(frag->data));
		bcmgenet_write(dev, desc + GENET_DMA_DESC_STATUS, len_ack);

		LOG_DBG("Tx fragment length: %d", frag->len);
		LOG_HEXDUMP_DBG(frag->data, frag->len, "Tx fragment");

		data->tx_idx++;
		if (data->tx_idx >= GENET_NB_TX_DESCS) {
			data->tx_idx = 0;
		}

		nb_frags++;
	}

	barrier_dmem_fence_full();

	data->pidx += nb_frags;
	bcmgenet_write(dev, GENET_TX_DMA_PIDX, data->pidx);

	if (!WAIT_FOR((bcmgenet_read(dev, GENET_TX_DMA_CIDX) & UINT16_MAX) >= data->pidx,
		      GENET_TX_TIMEOUT_US, k_busy_wait(10))) {
		LOG_WRN("Tx timeout at CIDX %d",
			bcmgenet_read(dev, GENET_TX_DMA_CIDX) & UINT16_MAX);
		return -ETIMEDOUT;
	}

	LOG_DBG("Tx index: %u, pidx: %u, cidx: %u", data->tx_idx, data->pidx,
		bcmgenet_read(dev, GENET_TX_DMA_CIDX));

	return 0;
}

static int bcmgenet_start(const struct device *dev, struct net_if *iface __unused)
{
	LOG_DBG("BCMGENET Started");

	/* Enable Rx/Tx DMA */
	bcmgenet_dma_ctrl(dev, true);

	/* Enable Rx/Tx queues */
	bcmgenet_ctrl(dev, true);

	return 0;
}

static int bcmgenet_stop(const struct device *dev, struct net_if *iface __unused)
{
	LOG_DBG("BCMGENET Stopped");

	/* Disable Rx/Tx queues */
	bcmgenet_ctrl(dev, false);

	/* Disable Rx/Tx DMA */
	bcmgenet_dma_ctrl(dev, false);

	return 0;
}

static void bcmgenet_set_mac_addr(const struct device *dev, const uint8_t *mac_addr)
{
	bcmgenet_write(dev, GENET_MAC_ADDR_LOW, sys_get_be32(&mac_addr[0]));
	bcmgenet_write(dev, GENET_MAC_ADDR_HIGH, (uint32_t)sys_get_be16(&mac_addr[4]));
}

static int bcmgenet_set_config(const struct device *dev, struct net_if *iface __unused,
			       enum ethernet_config_type type, const struct ethernet_config *config)
{
	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		bcmgenet_set_mac_addr(dev, config->mac_address.addr);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static enum ethernet_hw_caps bcmgenet_get_caps(const struct device *dev __unused,
					       struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_LINK_1000BASE;
}

static const struct device *bcmgenet_get_phy(const struct device *dev,
					     struct net_if *iface __unused)
{
	const struct bcmgenet_config *config = dev->config;

	return config->phy_dev;
}

static void phy_link_state_changed(const struct device *phy_dev __unused,
				   struct phy_link_state *state, void *user_data)
{
	struct net_if *iface = (struct net_if *)user_data;
	const struct device *dev = net_if_get_device(iface);
	uint8_t speed = 0;

	if (state->is_up) {
		if (PHY_LINK_IS_SPEED_1000M(state->speed)) {
			speed = GENET_MAC_SPEED_1000M;
		} else if (PHY_LINK_IS_SPEED_100M(state->speed)) {
			speed = GENET_MAC_SPEED_100M;
		} else if (PHY_LINK_IS_SPEED_10M(state->speed)) {
			speed = GENET_MAC_SPEED_10M;
		} else {
			LOG_WRN("PHY link speed is not supported");
			return;
		}

		if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
			bcmgenet_clear_bits(dev, GENET_MAC_CMD, GENET_MAC_HD_EN);
		} else {
			bcmgenet_set_bits(dev, GENET_MAC_CMD, GENET_MAC_HD_EN);
		}

		bcmgenet_clear_bits(dev, GENET_EXT_RGMII_OOB_CTRL, GENET_OOB_DISABLE);
		bcmgenet_set_bits(dev, GENET_EXT_RGMII_OOB_CTRL, GENET_RGMII_CFG);

		bcmgenet_clear_bits(dev, GENET_MAC_CMD, GENET_MAC_SPEED_MASK);
		bcmgenet_set_bits(dev, GENET_MAC_CMD, GENET_MAC_SPEED(speed));
	}

	net_eth_carrier_set(iface, state->is_up);
}

static void bcmgenet_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct bcmgenet_config *config = dev->config;
	struct bcmgenet_data *data = dev->data;
	const uint8_t *mac_addr = data->mac_addr;

	/* initialize ethernet L2 */
	ethernet_init(iface);
	data->iface = iface;

	net_if_set_link_addr(iface, mac_addr, NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);
	bcmgenet_set_mac_addr(dev, mac_addr);

	LOG_INF("%s: MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name, mac_addr[0],
		mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

	if (!device_is_ready(config->phy_dev)) {
		LOG_ERR_DEVICE_NOT_READY(config->phy_dev);
		return;
	}

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	phy_link_callback_set(config->phy_dev, phy_link_state_changed, (void *)iface);

	k_thread_create(
		&data->rx_thread, data->rx_thread_stack,
		K_KERNEL_STACK_SIZEOF(data->rx_thread_stack), bcmgenet_rx_thread, (void *)dev, NULL,
		NULL, K_PRIO_COOP(CONFIG_ETH_BCMGENET_RX_THREAD_PRIORITY), K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "eth_bcmgenet");
}

static const struct ethernet_api bcmgenet_api = {
	.iface_api.init = bcmgenet_iface_init,
	.get_phy = bcmgenet_get_phy,
	.get_capabilities = bcmgenet_get_caps,
	.set_config = bcmgenet_set_config,
	.start = bcmgenet_start,
	.stop = bcmgenet_stop,
	.send = bcmgenet_send,
};

static int bcmgenet_init(const struct device *dev)
{
	const struct bcmgenet_config *config = dev->config;
	struct bcmgenet_data *data = dev->data;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Flush Rx/Tx Buffers */
	bcmgenet_flush(dev);
	bcmgenet_reset(dev);

	/* Disable Rx/Tx DMA */
	bcmgenet_dma_ctrl(dev, false);

	/* Disable Rx/Tx queues */
	bcmgenet_ctrl(dev, false);

	/* Initialize Rx ring */
	ret = bcmgenet_rx_ring_init(dev);
	if (ret < 0) {
		return ret;
	}

	/* Initialize Tx ring */
	bcmgenet_tx_ring_init(dev);

	/* Set MTU and configure phy mode */
	bcmgenet_write(dev, GENET_MAC_MTU, GENET_MAX_FRAME_LEN);
	bcmgenet_write(dev, GENET_PORT_CTRL, GENET_PORT_MODE_EXT_GPHY);

	/* Load MAC address */
	(void)net_eth_mac_load(&config->mac_cfg, data->mac_addr);

	LOG_DBG("BCMGENET initialized");

	return 0;
}

BUILD_ASSERT(CONFIG_NET_BUF_RX_COUNT > GENET_BUF_RX_COUNT,
	     "Not enough Rx buffers count: need " STRINGIFY(GENET_BUF_RX_COUNT)
	     ", got " STRINGIFY(CONFIG_NET_BUF_RX_COUNT));

/* clang-format off */
#define BCMGENET_DEV_CFG(n)                                                                        \
	static const struct bcmgenet_config bcmgenet_##n##_cfg = {                                 \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.phy_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, phy_handle)),                  \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                                     \
	}

#define BCMGENET_DEV_DATA(n)                                                                       \
	static struct bcmgenet_data bcmgenet_##n##_data;

#define BCMGENET_INIT(n)                                                                           \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, bcmgenet_init, NULL, &bcmgenet_##n##_data,                \
				      &bcmgenet_##n##_cfg, CONFIG_ETH_INIT_PRIORITY,               \
				      &bcmgenet_api, GENET_MAX_FRAME_LEN)

#define BCMGENET_INSTANTIATE(inst)                                                                 \
	BCMGENET_DEV_CFG(inst);                                                                    \
	BCMGENET_DEV_DATA(inst);                                                                   \
	BCMGENET_INIT(inst);

DT_INST_FOREACH_STATUS_OKAY(BCMGENET_INSTANTIATE)
/* clang-format on */
