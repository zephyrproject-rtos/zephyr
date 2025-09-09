/*
 * Copyright (c) 2025 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensry_sy1xx_mac

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sy1xx_mac, CONFIG_ETHERNET_LOG_LEVEL);

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/phy.h>
#include <udma.h>
#include "eth.h"

/* MAC register offsets */
#define SY1XX_MAC_VERSION_REG      0x0000
#define SY1XX_MAC_ADDRESS_LOW_REG  0x0004
#define SY1XX_MAC_ADDRESS_HIGH_REG 0x0008
#define SY1XX_MAC_CTRL_REG         0x000c

/* MAC control register bit offsets */
#define SY1XX_MAC_CTRL_RESET_OFFS   (0)
#define SY1XX_MAC_CTRL_RX_EN_OFFS   (1)
#define SY1XX_MAC_CTRL_TX_EN_OFFS   (2)
#define SY1XX_MAC_CTRL_GMII_OFFS    (3)
#define SY1XX_MAC_CTRL_CLK_DIV_OFFS (8)
#define SY1XX_MAC_CTRL_CLK_SEL_OFFS (10)

/* MAC clock sources */
#define SY1XX_MAC_CTRL_CLK_SEL_REF_CLK 0
#define SY1XX_MAC_CTRL_CLK_SEL_MII_CLK 1

/* Clock divider options */
#define SY1XX_MAC_CTRL_CLK_DIV_1  0x0
#define SY1XX_MAC_CTRL_CLK_DIV_5  0x1
#define SY1XX_MAC_CTRL_CLK_DIV_10 0x2
#define SY1XX_MAC_CTRL_CLK_DIV_50 0x3

/* Clock divider mask */
#define SY1XX_MAC_CTRL_CLK_DIV_MASK (0x3)

#define MAX_MAC_PACKET_LEN      1600
#define MAX_TX_RETRIES          5
#define RECEIVE_GRACE_TIME_MSEC 1

#define SY1XX_ETH_STACK_SIZE      4096
#define SY1XX_ETH_THREAD_PRIORITY K_PRIO_PREEMPT(0)

struct sy1xx_mac_dev_config {
	/* address of controller configuration registers */
	uint32_t ctrl_addr;
	/* address of udma for data transfers */
	uint32_t base_addr;
	/* optional - enable promiscuous mode */
	bool promiscuous_mode;
	/* optional - random mac */
	bool use_zephyr_random_mac;

	/* phy config */
	const struct device *phy_dev;

	/* pinctrl for rgmii pins */
	const struct pinctrl_dev_config *pcfg;
};

struct sy1xx_mac_dma_buffers {
	uint8_t tx[MAX_MAC_PACKET_LEN];
	uint8_t rx[MAX_MAC_PACKET_LEN];
};

struct sy1xx_mac_dev_data {
	struct k_mutex mutex;

	/* current state of link and mac address */
	bool link_is_up;
	enum phy_link_speed link_speed;

	uint8_t mac_addr[6];

	/* intermediate, linear buffers that can hold a received or transmit msg */
	struct {
		uint8_t tx[MAX_MAC_PACKET_LEN];
		uint16_t tx_len;

		uint8_t rx[MAX_MAC_PACKET_LEN];
		uint16_t rx_len;
	} temp;

	/* buffers used for dma transfer, cannot be accessed while transfer active */
	struct sy1xx_mac_dma_buffers *dma_buffers;

	struct k_thread rx_data_thread;

	K_KERNEL_STACK_MEMBER(rx_data_thread_stack, SY1XX_ETH_STACK_SIZE);

	struct net_if *iface;
};

/* prototypes */
static int sy1xx_mac_set_mac_addr(const struct device *dev);
static int sy1xx_mac_set_promiscuous_mode(const struct device *dev, bool promiscuous_mode);
static int sy1xx_mac_set_config(const struct device *dev, enum ethernet_config_type type,
				const struct ethernet_config *config);
static void sy1xx_mac_rx_thread_entry(void *p1, void *p2, void *p3);

static int sy1xx_mac_initialize(const struct device *dev)
{
	struct sy1xx_mac_dev_config *cfg = (struct sy1xx_mac_dev_config *)dev->config;
	struct sy1xx_mac_dev_data *data = (struct sy1xx_mac_dev_data *)dev->data;
	int ret;

	data->link_is_up = false;
	data->link_speed = -1;

	k_mutex_init(&data->mutex);

	/* PAD config */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("failed to configure pins");
		return ret;
	}

	/* create the receiver thread */
	k_thread_create(&data->rx_data_thread, (k_thread_stack_t *)data->rx_data_thread_stack,
			SY1XX_ETH_STACK_SIZE, sy1xx_mac_rx_thread_entry, (void *)dev, NULL, NULL,
			SY1XX_ETH_THREAD_PRIORITY, 0, K_NO_WAIT);

	/* start suspended and resume once we have link */
	k_thread_suspend(&data->rx_data_thread);

	k_thread_name_set(&data->rx_data_thread, "mac-rx-thread");

	return 0;
}

static int sy1xx_mac_set_promiscuous_mode(const struct device *dev, bool promiscuous_mode)
{
	struct sy1xx_mac_dev_config *cfg = (struct sy1xx_mac_dev_config *)dev->config;
	uint32_t prom;

	/* set promiscuous mode */
	prom = sys_read32(cfg->ctrl_addr + SY1XX_MAC_ADDRESS_HIGH_REG);
	if (promiscuous_mode) {
		prom &= ~BIT(16);
	} else {
		prom |= BIT(16);
	}
	sys_write32(prom, cfg->ctrl_addr + SY1XX_MAC_ADDRESS_HIGH_REG);

	return 0;
}

static int sy1xx_mac_set_mac_addr(const struct device *dev)
{
	struct sy1xx_mac_dev_config *cfg = (struct sy1xx_mac_dev_config *)dev->config;
	struct sy1xx_mac_dev_data *data = (struct sy1xx_mac_dev_data *)dev->data;
	int ret;
	uint32_t v_low, v_high;

	LOG_INF("%s set link address %02x:%02x:%02x:%02x:%02x:%02x", dev->name, data->mac_addr[0],
		data->mac_addr[1], data->mac_addr[2], data->mac_addr[3], data->mac_addr[4],
		data->mac_addr[5]);

	/* update mac in controller */
	v_low = sys_get_le32(&data->mac_addr[0]);
	sys_write32(v_low, cfg->ctrl_addr + SY1XX_MAC_ADDRESS_LOW_REG);

	v_high = sys_read32(cfg->ctrl_addr + SY1XX_MAC_ADDRESS_HIGH_REG);
	v_high |= (v_high & 0xffff0000) | sys_get_le16(&data->mac_addr[4]);
	sys_write32(v_high, cfg->ctrl_addr + SY1XX_MAC_ADDRESS_HIGH_REG);

	/* Register Ethernet MAC Address with the upper layer */
	ret = net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				   NET_LINK_ETHERNET);
	if (ret) {
		LOG_ERR("%s failed to set link address", dev->name);
		return ret;
	}

	return 0;
}

static int sy1xx_mac_start(const struct device *dev)
{
	struct sy1xx_mac_dev_config *cfg = (struct sy1xx_mac_dev_config *)dev->config;
	struct sy1xx_mac_dev_data *data = (struct sy1xx_mac_dev_data *)dev->data;

	extern void sy1xx_udma_disable_clock(sy1xx_udma_module_t module, uint32_t instance);

	/* UDMA clock reset and enable */
	sy1xx_udma_enable_clock(SY1XX_UDMA_MODULE_MAC, 0);
	sy1xx_udma_disable_clock(SY1XX_UDMA_MODULE_MAC, 0);
	sy1xx_udma_enable_clock(SY1XX_UDMA_MODULE_MAC, 0);

	/* reset mac controller */
	sys_write32(0x0001, cfg->ctrl_addr + SY1XX_MAC_CTRL_REG);
	sys_write32(0x0000, cfg->ctrl_addr + SY1XX_MAC_CTRL_REG);

	if (cfg->use_zephyr_random_mac) {
		/* prio 1 -- generate random, if set in device tree */
		sys_rand_get(&data->mac_addr, 6);
		/* Set MAC address locally administered, unicast (LAA) */
		data->mac_addr[0] |= 0x02;
	}

	sy1xx_mac_set_mac_addr(dev);

	sy1xx_mac_set_promiscuous_mode(dev, cfg->promiscuous_mode);

	k_thread_resume(&data->rx_data_thread);

	return 0;
}

static int sy1xx_mac_stop(const struct device *dev)
{
	struct sy1xx_mac_dev_data *data = (struct sy1xx_mac_dev_data *)dev->data;

	k_thread_suspend(&data->rx_data_thread);
	return 0;
}

static void phy_link_state_changed(const struct device *pdev, struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct sy1xx_mac_dev_data *const data = dev->data;
	const struct sy1xx_mac_dev_config *const cfg = dev->config;
	bool is_up;
	enum phy_link_speed speed;
	uint32_t v, en;

	is_up = state->is_up;
	speed = state->speed;

	if (speed != data->link_speed) {
		data->link_speed = speed;

		/* configure mac, based on provided link information 1Gbs/100MBit/... */
		v = sys_read32(cfg->ctrl_addr + SY1XX_MAC_CTRL_REG);

		/* mask out relevant bits */
		v &= ~(BIT(SY1XX_MAC_CTRL_GMII_OFFS) | BIT(SY1XX_MAC_CTRL_CLK_SEL_OFFS) |
		       (SY1XX_MAC_CTRL_CLK_DIV_MASK << SY1XX_MAC_CTRL_CLK_DIV_OFFS));

		switch (speed) {
		case LINK_FULL_10BASE:
			LOG_INF("link speed FULL_10BASE_T");
			/* 2.5MHz, MAC is clock source */
			v |= (SY1XX_MAC_CTRL_CLK_SEL_MII_CLK << SY1XX_MAC_CTRL_CLK_SEL_OFFS) |
			     (SY1XX_MAC_CTRL_CLK_DIV_10 << SY1XX_MAC_CTRL_CLK_DIV_OFFS);
			break;
		case LINK_FULL_100BASE:
			LOG_INF("link speed FULL_100BASE_T");
			/* 25MHz, MAC is clock source */
			v |= (SY1XX_MAC_CTRL_CLK_SEL_MII_CLK << SY1XX_MAC_CTRL_CLK_SEL_OFFS) |
			     (SY1XX_MAC_CTRL_CLK_DIV_1 << SY1XX_MAC_CTRL_CLK_DIV_OFFS);
			break;
		case LINK_FULL_1000BASE:
			LOG_INF("link speed FULL_1000BASE_T");
			/* 125MHz, Phy is clock source */
			v |= BIT(SY1XX_MAC_CTRL_GMII_OFFS) |
			     (SY1XX_MAC_CTRL_CLK_SEL_REF_CLK << SY1XX_MAC_CTRL_CLK_SEL_OFFS) |
			     (SY1XX_MAC_CTRL_CLK_DIV_1 << SY1XX_MAC_CTRL_CLK_DIV_OFFS);
			break;
		default:
			LOG_ERR("invalid link speed");
			return;
		}
		sys_write32(v, cfg->ctrl_addr + SY1XX_MAC_CTRL_REG);
	}

	if (is_up != data->link_is_up) {
		data->link_is_up = is_up;

		if (is_up) {
			LOG_DBG("Link up");

			/* enable mac controller */
			en = sys_read32(cfg->ctrl_addr + SY1XX_MAC_CTRL_REG);
			en |= BIT(SY1XX_MAC_CTRL_TX_EN_OFFS) | BIT(SY1XX_MAC_CTRL_RX_EN_OFFS);
			sys_write32(en, cfg->ctrl_addr + SY1XX_MAC_CTRL_REG);

			/* Announce link up status */
			net_eth_carrier_on(data->iface);

		} else {
			LOG_DBG("Link down");

			/* disable mac controller */
			en = sys_read32(cfg->ctrl_addr + SY1XX_MAC_CTRL_REG);
			en &= ~(BIT(SY1XX_MAC_CTRL_TX_EN_OFFS) | BIT(SY1XX_MAC_CTRL_RX_EN_OFFS));
			sys_write32(en, cfg->ctrl_addr + SY1XX_MAC_CTRL_REG);

			/* Announce link down status */
			net_eth_carrier_off(data->iface);
		}
	}
}

static void sy1xx_mac_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct sy1xx_mac_dev_config *cfg = (struct sy1xx_mac_dev_config *)dev->config;
	struct sy1xx_mac_dev_data *const data = dev->data;

	LOG_INF("Interface init %s (%p)", net_if_get_device(iface)->name, iface);

	data->iface = iface;

	ethernet_init(iface);

	if (device_is_ready(cfg->phy_dev)) {
		phy_link_callback_set(cfg->phy_dev, &phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}

	/* Do not start the interface until PHY link is up */
	if (!(data->link_is_up)) {
		LOG_INF("found PHY link down");
		net_if_carrier_off(iface);
	}
}

static enum ethernet_hw_caps sy1xx_mac_get_caps(const struct device *dev)
{
	enum ethernet_hw_caps supported = 0;

	/* basic implemented features */
	supported |= ETHERNET_PROMISC_MODE;
	supported |= ETHERNET_LINK_1000BASE;
	supported |= ETHERNET_PROMISC_MODE;

	return supported;
}

static int sy1xx_mac_set_config(const struct device *dev, enum ethernet_config_type type,
				const struct ethernet_config *config)
{
	struct sy1xx_mac_dev_data *data = (struct sy1xx_mac_dev_data *)dev->data;
	int ret = 0;

	switch (type) {

	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		ret = sy1xx_mac_set_promiscuous_mode(dev, config->promisc_mode);
		break;

	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		ret = sy1xx_mac_set_mac_addr(dev);
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}

static const struct device *sy1xx_mac_get_phy(const struct device *dev)
{
	const struct sy1xx_mac_dev_config *const cfg = dev->config;

	return cfg->phy_dev;
}

/*
 * rx ready status of eth is different to any other rx udma,
 * so we implement here
 */
static int32_t sy1xx_mac_udma_is_finished_rx(uint32_t base)
{
	uint32_t isBusy = SY1XX_UDMA_READ_REG(base, SY1XX_UDMA_CFG_REG + 0x00) & (BIT(17));

	return isBusy ? 0 : 1;
}

static int sy1xx_mac_low_level_send(const struct device *dev, uint8_t *tx, uint16_t len)
{
	struct sy1xx_mac_dev_config *cfg = (struct sy1xx_mac_dev_config *)dev->config;
	struct sy1xx_mac_dev_data *const data = dev->data;

	if (len == 0) {
		return -EINVAL;
	}

	if (len > MAX_MAC_PACKET_LEN) {
		return -EINVAL;
	}

	if (!SY1XX_UDMA_IS_FINISHED_TX(cfg->base_addr)) {
		return -EBUSY;
	}

	/* udma is ready, double check if last transmission was successful */
	if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base_addr)) {
		SY1XX_UDMA_CANCEL_TX(cfg->base_addr);
		LOG_ERR("tx - last transmission failed");
		return -EINVAL;
	}

	/* copy data to dma buffer */
	for (uint32_t i = 0; i < len; i++) {
		data->dma_buffers->tx[i] = tx[i];
	}

	/* start dma transfer */
	SY1XX_UDMA_START_TX(cfg->base_addr, (uint32_t)data->dma_buffers->tx, len, 0);

	return 0;
}

static int sy1xx_mac_low_level_receive(const struct device *dev, uint8_t *rx, uint16_t *len)
{
	struct sy1xx_mac_dev_config *cfg = (struct sy1xx_mac_dev_config *)dev->config;
	struct sy1xx_mac_dev_data *const data = dev->data;
	int ret;
	uint32_t bytes_transferred;

	*len = 0;

	/* rx udma still busy */
	if (0 == sy1xx_mac_udma_is_finished_rx(cfg->base_addr)) {
		return -EBUSY;
	}

	/* rx udma is ready */
	bytes_transferred = SY1XX_UDMA_READ_REG(cfg->base_addr, SY1XX_UDMA_CFG_REG) & 0x0000ffff;
	if (bytes_transferred) {
		/* message received, copy data */
		memcpy(rx, data->dma_buffers->rx, bytes_transferred);
		*len = bytes_transferred;
		ret = 0;
	} else {
		/* no data, should never happen */
		SY1XX_UDMA_CANCEL_RX(cfg->base_addr);
		ret = -EINVAL;
	}

	/* start new reception */
	SY1XX_UDMA_START_RX(cfg->base_addr, (uint32_t)data->dma_buffers->rx, MAX_MAC_PACKET_LEN, 0);

	return ret;
}

static int sy1xx_mac_send(const struct device *dev, struct net_pkt *pkt)
{
	struct sy1xx_mac_dev_data *const data = dev->data;
	int ret;
	uint32_t retries_left;
	struct net_buf *frag;

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* push all fragments of the packet into one linear buffer */
	frag = pkt->buffer;
	data->temp.tx_len = 0;
	do {
		/* copy fragment to buffer */
		for (uint32_t i = 0; i < frag->len; i++) {
			if (data->temp.tx_len < MAX_MAC_PACKET_LEN) {
				data->temp.tx[data->temp.tx_len++] = frag->data[i];
			} else {
				LOG_ERR("tx buffer overflow");
				k_mutex_unlock(&data->mutex);
				return -ENOMEM;
			}
		}

		frag = frag->frags;
	} while (frag);

	/* hand over linear tx frame to udma */
	retries_left = MAX_TX_RETRIES;
	while (retries_left) {
		ret = sy1xx_mac_low_level_send(dev, data->temp.tx, data->temp.tx_len);
		if (ret == 0) {
			break;
		}
		if (ret != -EBUSY) {
			LOG_ERR("tx error");
			k_mutex_unlock(&data->mutex);
			return ret;
		}
		k_sleep(K_MSEC(1));
		retries_left--;
	};

	k_mutex_unlock(&data->mutex);
	return ret;
}

static int sy1xx_mac_receive_data(const struct device *dev, uint8_t *rx, uint16_t len)
{
	struct sy1xx_mac_dev_data *const data = dev->data;
	struct net_pkt *rx_pkt;
	int ret;

	rx_pkt = net_pkt_alloc_with_buffer(data->iface, len, AF_UNSPEC, 0, K_FOREVER);
	if (rx_pkt == NULL) {
		LOG_ERR("rx packet allocation failed");
		return -EINVAL;
	}

	/* add data to the net_pkt */
	if (net_pkt_write(rx_pkt, rx, len)) {
		LOG_ERR("failed to write data to net_pkt");
		net_pkt_unref(rx_pkt);
		return -EINVAL;
	}

	/* register new packet in stack */
	ret = net_recv_data(data->iface, rx_pkt);
	if (ret) {
		LOG_ERR("rx packet registration failed");
		return ret;
	}

	return 0;
}

static void sy1xx_mac_rx_thread_entry(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct sy1xx_mac_dev_data *const data = dev->data;
	int ret;

	while (true) {
		ret = sy1xx_mac_low_level_receive(dev, data->temp.rx, &data->temp.rx_len);
		if (ret == 0) {
			/* register a new received frame */
			if (data->temp.rx_len) {
				sy1xx_mac_receive_data(dev, data->temp.rx, data->temp.rx_len);
			}
		} else {
			/*
			 * rx thread is running at higher prio, in case of error or busy,
			 * we could lock up the system partially.
			 */

			/* we wait and try again */
			k_sleep(K_MSEC(RECEIVE_GRACE_TIME_MSEC));
		}
	}
}

const struct ethernet_api sy1xx_mac_driver_api = {
	.start = sy1xx_mac_start,
	.stop = sy1xx_mac_stop,
	.iface_api.init = sy1xx_mac_iface_init,
	.get_capabilities = sy1xx_mac_get_caps,
	.set_config = sy1xx_mac_set_config,
	.send = sy1xx_mac_send,
	.get_phy = sy1xx_mac_get_phy,
};

#define SY1XX_MAC_INIT(n)                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct sy1xx_mac_dev_config sy1xx_mac_dev_config_##n = {                      \
		.ctrl_addr = DT_INST_REG_ADDR_BY_NAME(n, ctrl),                                    \
		.base_addr = DT_INST_REG_ADDR_BY_NAME(n, data),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.promiscuous_mode = DT_INST_PROP_OR(n, promiscuous_mode, false),                   \
		.use_zephyr_random_mac = DT_INST_PROP(n, zephyr_random_mac_address),               \
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle))};                         \
                                                                                                   \
	static struct sy1xx_mac_dma_buffers __attribute__((section(".udma_access")))               \
	__aligned(4) sy1xx_mac_dma_buffers_##n;                                                    \
                                                                                                   \
	static struct sy1xx_mac_dev_data sy1xx_mac_dev_data##n = {                                 \
		.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),                            \
		.dma_buffers = &sy1xx_mac_dma_buffers_##n,                                         \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, &sy1xx_mac_initialize, NULL, &sy1xx_mac_dev_data##n,      \
				      &sy1xx_mac_dev_config_##n, CONFIG_ETH_INIT_PRIORITY,         \
				      &sy1xx_mac_driver_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(SY1XX_MAC_INIT)
