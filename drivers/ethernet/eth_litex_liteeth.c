/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_liteeth

#define LOG_MODULE_NAME eth_litex_liteeth
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <stdbool.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>

#include <zephyr/sys/printk.h>
#include <zephyr/irq.h>

#include "eth.h"

#define MAX_TX_FAILURE K_MSEC(100)

#define ETH_LITEX_SLOT_SIZE 0x0800

struct eth_litex_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	uint8_t txslot;
	struct k_mutex tx_mutex;
	struct k_sem sem_tx_ready;
};

struct eth_litex_config {
	const struct device *phy_dev;
	void (*config_func)(const struct device *dev);
	bool random_mac_address;
	uint32_t rx_slot_addr;
	uint32_t rx_length_addr;
	uint32_t rx_ev_pending_addr;
	uint32_t rx_ev_enable_addr;
	uint32_t tx_start_addr;
	uint32_t tx_ready_addr;
	uint32_t tx_slot_addr;
	uint32_t tx_length_addr;
	uint32_t tx_ev_pending_addr;
	uint32_t tx_ev_enable_addr;
	uint32_t tx_buf_addr;
	uint32_t rx_buf_addr;
	uint8_t tx_buf_n;
	uint8_t rx_buf_n;
};

static int eth_initialize(const struct device *dev)
{
	const struct eth_litex_config *config = dev->config;
	struct eth_litex_dev_data *context = dev->data;

	k_mutex_init(&context->tx_mutex);
	k_sem_init(&context->sem_tx_ready, 1, 1);

	config->config_func(dev);

	if (config->random_mac_address) {
		/* generate random MAC address */
		gen_random_mac(context->mac_addr, 0x10, 0xe2, 0xd5);
	}

	return 0;
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	uint16_t len;
	struct eth_litex_dev_data *context = dev->data;
	const struct eth_litex_config *config = dev->config;
	int ret;

	k_mutex_lock(&context->tx_mutex, K_FOREVER);

	/* get data from packet and send it */
	len = net_pkt_get_len(pkt);
	net_pkt_read(pkt,
		     UINT_TO_POINTER(config->tx_buf_addr + (context->txslot * ETH_LITEX_SLOT_SIZE)),
		     len);

	litex_write8(context->txslot, config->tx_slot_addr);
	litex_write16(len, config->tx_length_addr);

	/* wait for the device to be ready to transmit */
	ret = k_sem_take(&context->sem_tx_ready, MAX_TX_FAILURE);
	if (ret < 0) {
		goto error;
	};
	/* start transmitting */
	litex_write8(1, config->tx_start_addr);

	/* change slot */
	context->txslot = (context->txslot + 1) % config->tx_buf_n;

	k_mutex_unlock(&context->tx_mutex);

	return 0;
error:
	k_mutex_unlock(&context->tx_mutex);
	LOG_ERR("TX fifo failed");
	return -EIO;
}

static void eth_rx(const struct device *port)
{
	struct net_pkt *pkt;
	struct eth_litex_dev_data *context = port->data;
	const struct eth_litex_config *config = port->config;

	int r;
	uint16_t len = 0;
	uint8_t rxslot = 0;

	if (!net_if_flag_is_set(context->iface, NET_IF_UP)) {
		return;
	}

	/* get frame's length */
	len = litex_read16(config->rx_length_addr);

	/* which slot is the frame in */
	rxslot = litex_read8(config->rx_slot_addr);

	/* obtain rx buffer */
	pkt = net_pkt_rx_alloc_with_buffer(context->iface, len, AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("Failed to obtain RX buffer");
		return;
	}

	/* copy data to buffer */
	if (net_pkt_write(pkt,
			  UINT_TO_POINTER(config->rx_buf_addr + (rxslot * ETH_LITEX_SLOT_SIZE)),
			  len) != 0) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		return;
	}

	/* receive data */
	r = net_recv_data(context->iface, pkt);
	if (r < 0) {
		LOG_ERR("Failed to enqueue frame into RX queue: %d", r);
		net_pkt_unref(pkt);
	}
}

static void eth_irq_handler(const struct device *port)
{
	struct eth_litex_dev_data *context = port->data;
	const struct eth_litex_config *config = port->config;
	/* check sram reader events (tx) */
	if (litex_read8(config->tx_ev_pending_addr) & BIT(0)) {
		k_sem_give(&context->sem_tx_ready);

		/* ack reader irq */
		litex_write8(BIT(0), config->tx_ev_pending_addr);
	}

	/* check sram writer events (rx) */
	if (litex_read8(config->rx_ev_pending_addr) & BIT(0)) {
		eth_rx(port);

		/* ack writer irq */
		litex_write8(BIT(0), config->rx_ev_pending_addr);
	}
}

static int eth_set_config(const struct device *dev, enum ethernet_config_type type,
			  const struct ethernet_config *config)
{
	struct eth_litex_dev_data *context = dev->data;
	int ret = -ENOTSUP;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(context->mac_addr, config->mac_address.addr, sizeof(context->mac_addr));
		ret = net_if_set_link_addr(context->iface, context->mac_addr,
					   sizeof(context->mac_addr), NET_LINK_ETHERNET);
		break;
	default:
		break;
	}

	return ret;
}

static int eth_start(const struct device *dev)
{
	struct eth_litex_dev_data *context = dev->data;
	const struct eth_litex_config *config = dev->config;

	if (litex_read8(config->tx_ready_addr)) {
		k_sem_give(&context->sem_tx_ready);
	}

	litex_write8(1, config->tx_ev_enable_addr);
	litex_write8(1, config->rx_ev_enable_addr);

	litex_write8(BIT(0), config->tx_ev_pending_addr);
	litex_write8(BIT(0), config->rx_ev_pending_addr);

	return 0;
}

static int eth_stop(const struct device *dev)
{
	const struct eth_litex_config *config = dev->config;

	litex_write8(0, config->tx_ev_enable_addr);
	litex_write8(0, config->rx_ev_enable_addr);

	return 0;
}

static const struct device *eth_get_phy(const struct device *dev)
{
	const struct eth_litex_config *config = dev->config;

	return config->phy_dev;
}

static void phy_link_state_changed(const struct device *phy_dev,
				   struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct eth_litex_dev_data *context = dev->data;

	ARG_UNUSED(phy_dev);

	if (state->is_up) {
		net_eth_carrier_on(context->iface);
	} else {
		net_eth_carrier_off(context->iface);
	}
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *port = net_if_get_device(iface);
	const struct eth_litex_config *config = port->config;
	struct eth_litex_dev_data *context = port->data;

	/* set interface */
	if (context->iface == NULL) {
		context->iface = iface;
	}

	/* initialize ethernet L2 */
	ethernet_init(iface);

	/* set MAC address */
	if (net_if_set_link_addr(iface, context->mac_addr, sizeof(context->mac_addr),
			     NET_LINK_ETHERNET) < 0) {
		LOG_ERR("setting mac failed");
		return;
	}

	if (config->phy_dev == NULL) {
		LOG_WRN("No PHY device");
		return;
	}

	net_if_carrier_off(iface);

	if (device_is_ready(config->phy_dev)) {
		phy_link_callback_set(config->phy_dev, phy_link_state_changed, (void *)port);
	} else {
		LOG_ERR("PHY device not ready");
	}
}

static enum ethernet_hw_caps eth_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return
#ifdef CONFIG_NET_VLAN
		ETHERNET_HW_VLAN |
#endif
		ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T | ETHERNET_LINK_1000BASE_T;
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.start = eth_start,
	.stop = eth_stop,
	.get_capabilities = eth_caps,
	.set_config = eth_set_config,
	.get_phy = eth_get_phy,
	.send = eth_tx
};

#define ETH_LITEX_SLOT_RX_ADDR(n)                                                                  \
	DT_INST_REG_ADDR_BY_NAME_OR(n, rx_buffers, (DT_INST_REG_ADDR_BY_NAME(n, buffers)))
#define ETH_LITEX_SLOT_TX_ADDR(n)                                                                  \
	DT_INST_REG_ADDR_BY_NAME_OR(n, tx_buffers,                                                 \
				    (DT_INST_REG_ADDR_BY_NAME(n, buffers) +                        \
				     (DT_INST_REG_SIZE_BY_NAME(n, buffers) / 2)))
#define ETH_LITEX_SLOT_RX_N(n)                                                                     \
	(DT_INST_REG_SIZE_BY_NAME_OR(n, rx_buffers, (DT_INST_REG_SIZE_BY_NAME(n, buffers) / 2)) /  \
	 ETH_LITEX_SLOT_SIZE)
#define ETH_LITEX_SLOT_TX_N(n)                                                                     \
	(DT_INST_REG_SIZE_BY_NAME_OR(n, tx_buffers, (DT_INST_REG_SIZE_BY_NAME(n, buffers) / 2)) /  \
	 ETH_LITEX_SLOT_SIZE)

#define ETH_LITEX_INIT(n)                                                                          \
                                                                                                   \
	static void eth_irq_config##n(const struct device *dev)                                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), eth_irq_handler,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static struct eth_litex_dev_data eth_data##n = {                                           \
		.mac_addr = DT_INST_PROP(n, local_mac_address)};                                   \
                                                                                                   \
	static const struct eth_litex_config eth_config##n = {                                     \
		.phy_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, phy_handle)),                  \
		.config_func = eth_irq_config##n,                                                  \
		.random_mac_address = DT_INST_PROP(n, zephyr_random_mac_address),                  \
		.rx_slot_addr = DT_INST_REG_ADDR_BY_NAME(n, rx_slot),                              \
		.rx_length_addr = DT_INST_REG_ADDR_BY_NAME(n, rx_length),                          \
		.rx_ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, rx_ev_pending),                  \
		.rx_ev_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, rx_ev_enable),                    \
		.tx_start_addr = DT_INST_REG_ADDR_BY_NAME(n, tx_start),                            \
		.tx_ready_addr = DT_INST_REG_ADDR_BY_NAME(n, tx_ready),                            \
		.tx_slot_addr = DT_INST_REG_ADDR_BY_NAME(n, tx_slot),                              \
		.tx_length_addr = DT_INST_REG_ADDR_BY_NAME(n, tx_length),                          \
		.tx_ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, tx_ev_pending),                  \
		.tx_ev_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, tx_ev_enable),                    \
		.rx_buf_addr = ETH_LITEX_SLOT_RX_ADDR(n),                                          \
		.tx_buf_addr = ETH_LITEX_SLOT_TX_ADDR(n),                                          \
		.rx_buf_n = ETH_LITEX_SLOT_RX_N(n),                                                \
		.tx_buf_n = ETH_LITEX_SLOT_TX_N(n),                                                \
                                                                                                   \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_initialize, NULL, &eth_data##n, &eth_config##n,       \
				      CONFIG_ETH_INIT_PRIORITY, &eth_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_LITEX_INIT);
