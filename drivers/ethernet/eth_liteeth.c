/*
 * Copyright (c) 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME eth_liteeth
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <stdbool.h>
#include <net/ethernet.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <misc/printk.h>

/* flags */
#define LITEETH_EV_TX		0x1
#define LITEETH_EV_RX		0x1

/* slots */
#define LITEETH_SLOT_BASE	DT_INST_0_LITEX_ETH0_BUFFERS_BASE_ADDRESS
#define LITEETH_SLOT_RX0	((LITEETH_SLOT_BASE) + 0x0000)
#define LITEETH_SLOT_RX1	((LITEETH_SLOT_BASE) + 0x0800)
#define LITEETH_SLOT_TX0	((LITEETH_SLOT_BASE) + 0x1000)
#define LITEETH_SLOT_TX1	((LITEETH_SLOT_BASE) + 0x1800)

/* sram - rx */
#define LITEETH_RX_BASE		DT_INST_0_LITEX_ETH0_CONTROL_BASE_ADDRESS
#define LITEETH_RX_SLOT		((LITEETH_RX_BASE) + 0x00)
#define LITEETH_RX_LENGTH	((LITEETH_RX_BASE) + 0x04)
#define LITEETH_RX_EV_PENDING	((LITEETH_RX_BASE) + 0x28)
#define LITEETH_RX_EV_ENABLE	((LITEETH_RX_BASE) + 0x2c)

/* sram - tx */
#define LITEETH_TX_BASE		((DT_INST_0_LITEX_ETH0_CONTROL_BASE_ADDRESS) + 0x30)
#define LITEETH_TX_START	((LITEETH_TX_BASE) + 0x00)
#define LITEETH_TX_READY	((LITEETH_TX_BASE) + 0x04)
#define LITEETH_TX_SLOT		((LITEETH_TX_BASE) + 0x0c)
#define LITEETH_TX_LENGTH	((LITEETH_TX_BASE) + 0x10)
#define LITEETH_TX_EV_PENDING	((LITEETH_TX_BASE) + 0x1c)

/* irq */
#define LITEETH_IRQ		DT_INST_0_LITEX_ETH0_IRQ_0
#define LITEETH_IRQ_PRIORITY	CONFIG_ETH_LITEETH_0_IRQ_PRI

/* label */
#define LITEETH_LABEL		DT_INST_0_LITEX_ETH0_LABEL

struct eth_liteeth_dev_data {
	struct net_if *iface;
	u8_t mac_addr[6];

	u8_t txslot;
	u8_t rxslot;

	u8_t *tx_buf[2];
	u8_t *rx_buf[2];
};

struct eth_liteeth_config {
	void (*config_func)(void);
};

static int eth_initialize(struct device *dev)
{
	const struct eth_liteeth_config *config = dev->config->config_info;

	config->config_func();

	return 0;
}

static int eth_tx(struct device *dev, struct net_pkt *pkt)
{
	int key;
	u16_t len;
	struct eth_liteeth_dev_data *context = dev->driver_data;

	key = irq_lock();

	/* get data from packet and send it */
	len = net_pkt_get_len(pkt);
	net_pkt_read(pkt, context->tx_buf[context->txslot], len);

	sys_write8(context->txslot, LITEETH_TX_SLOT);
	sys_write8(len >> 8, LITEETH_TX_LENGTH);
	sys_write8(len & 0xFF, LITEETH_TX_LENGTH + 4);

	/* wait for the device to be ready to transmit */
	while (sys_read8(LITEETH_TX_READY) == 0) {
		;
	}

	/* start transmitting */
	sys_write8(1, LITEETH_TX_START);

	/* change slot */
	context->txslot = (context->txslot + 1) % 2;

	irq_unlock(key);

	return 0;
}

static void eth_rx(struct device *port)
{
	struct net_pkt *pkt;
	struct eth_liteeth_dev_data *context = port->driver_data;

	unsigned int key, r;
	u16_t len = 0;

	key = irq_lock();

	/* get frame's length */
	for (int i = 0; i < 4; i++) {
		len <<= 8;
		len |= sys_read8(LITEETH_RX_LENGTH + i * 0x4);
	}

	/* which slot is the frame in */
	context->rxslot = sys_read8(LITEETH_RX_SLOT);

	/* obtain rx buffer */
	pkt = net_pkt_rx_alloc_with_buffer(context->iface, len, AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("Failed to obtain RX buffer");
		goto out;
	}

	/* copy data to buffer */
	if (net_pkt_write(pkt, (void *)context->rx_buf[context->rxslot], len) != 0) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		goto out;
	}

	/* receive data */
	r = net_recv_data(context->iface, pkt);
	if (r < 0) {
		LOG_ERR("Failed to enqueue frame into RX queue: %d", r);
		net_pkt_unref(pkt);
	}

out:
	irq_unlock(key);
}

static void eth_irq_handler(struct device *port)
{
	/* check sram reader events (tx) */
	if (sys_read8(LITEETH_TX_EV_PENDING) & LITEETH_EV_TX) {
		/* TX event is not enabled nor used by this driver; ack just
		 * in case if some rogue TX event appeared
		 */
		sys_write8(LITEETH_EV_TX, LITEETH_TX_EV_PENDING);
	}

	/* check sram writer events (rx) */
	if (sys_read8(LITEETH_RX_EV_PENDING) & LITEETH_EV_RX) {
		eth_rx(port);

		/* ack writer irq */
		sys_write8(LITEETH_EV_RX, LITEETH_RX_EV_PENDING);
	}
}

#ifdef CONFIG_ETH_LITEETH_0_RANDOM_MAC
static void generate_mac(u8_t *mac_addr)
{
	u32_t entropy;

	entropy = sys_rand32_get();

	mac_addr[3] = entropy >> 8;
	mac_addr[4] = entropy >> 16;
	/* Locally administered, unicast */
	mac_addr[5] = ((entropy >> 0) & 0xfc) | 0x02;
}
#endif

#ifdef CONFIG_ETH_LITEETH_0

static struct eth_liteeth_dev_data eth_data = {
	.mac_addr =  DT_INST_0_LITEX_ETH0_LOCAL_MAC_ADDRESS
};

static void eth_irq_config(void);
static const struct eth_liteeth_config eth_config = {
	.config_func = eth_irq_config,
};

static void eth_iface_init(struct net_if *iface)
{
	struct device *port = net_if_get_device(iface);
	struct eth_liteeth_dev_data *context = port->driver_data;
	static bool init_done;

	/* initialize only once */
	if (init_done) {
		return;
	}

	/* set interface */
	context->iface = iface;

	/* initialize ethernet L2 */
	ethernet_init(iface);

#ifdef CONFIG_ETH_LITEETH_0_RANDOM_MAC
	/* generate random MAC address */
	generate_mac(context->mac_addr);
#endif

	/* set MAC address */
	net_if_set_link_addr(iface, context->mac_addr, sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	/* clear pending events */
	sys_write8(LITEETH_EV_TX, LITEETH_TX_EV_PENDING);
	sys_write8(LITEETH_EV_RX, LITEETH_RX_EV_PENDING);

	/* setup tx slots */
	context->txslot = 0;
	context->tx_buf[0] = (u8_t *)LITEETH_SLOT_TX0;
	context->tx_buf[1] = (u8_t *)LITEETH_SLOT_TX1;

	/* setup rx slots */
	context->rxslot = 0;
	context->rx_buf[0] = (u8_t *)LITEETH_SLOT_RX0;
	context->rx_buf[1] = (u8_t *)LITEETH_SLOT_RX1;

	init_done = true;
}

static enum ethernet_hw_caps eth_caps(struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T |
	       ETHERNET_LINK_1000BASE_T;
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.get_capabilities = eth_caps,
	.send = eth_tx
};

NET_DEVICE_INIT(eth0, LITEETH_LABEL, eth_initialize, &eth_data, &eth_config,
		CONFIG_ETH_INIT_PRIORITY, &eth_api, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

static void eth_irq_config(void)
{
	IRQ_CONNECT(LITEETH_IRQ, LITEETH_IRQ_PRIORITY, eth_irq_handler,
		    DEVICE_GET(eth0), 0);
	irq_enable(LITEETH_IRQ);
	sys_write8(1, LITEETH_RX_EV_ENABLE);
}

#endif /* CONFIG_ETH_LITEETH_0 */
