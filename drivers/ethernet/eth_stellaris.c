/* Stellaris Ethernet Controller
 *
 * Copyright (c) 2018 Zilogic Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_stellaris_ethernet

#define LOG_MODULE_NAME eth_stellaris
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <net/ethernet.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <device.h>
#include <soc.h>
#include <ethernet/eth_stats.h>
#include "eth_stellaris_priv.h"

static void eth_stellaris_assign_mac(const struct device *dev)
{
	uint8_t mac_addr[6] = DT_INST_PROP(0, local_mac_address);
	uint32_t value = 0x0;

	value |= mac_addr[0];
	value |= mac_addr[1] << 8;
	value |= mac_addr[2] << 16;
	value |= mac_addr[3] << 24;
	sys_write32(value, REG_MACIA0);

	value = 0x0;
	value |= mac_addr[4];
	value |= mac_addr[5] << 8;
	sys_write32(value, REG_MACIA1);
}

static void eth_stellaris_flush(const struct device *dev)
{
	struct eth_stellaris_runtime *dev_data = DEV_DATA(dev);

	if (dev_data->tx_pos != 0) {
		sys_write32(dev_data->tx_word, REG_MACDATA);
		dev_data->tx_pos = 0;
		dev_data->tx_word = 0U;
	}
}

static void eth_stellaris_send_byte(const struct device *dev, uint8_t byte)
{
	struct eth_stellaris_runtime *dev_data = DEV_DATA(dev);

	dev_data->tx_word |= byte << (dev_data->tx_pos * 8);
	dev_data->tx_pos++;
	if (dev_data->tx_pos == 4) {
		sys_write32(dev_data->tx_word, REG_MACDATA);
		dev_data->tx_pos = 0;
		dev_data->tx_word = 0U;
	}
}

static int eth_stellaris_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_stellaris_runtime *dev_data = DEV_DATA(dev);
	struct net_buf *frag;
	uint16_t i, data_len;

	/* Frame transmission
	 *
	 * First two bytes is the length of the frame, exclusive of
	 * the header length.
	 */
	data_len = net_pkt_get_len(pkt) - sizeof(struct net_eth_hdr);
	eth_stellaris_send_byte(dev, data_len & 0xff);
	eth_stellaris_send_byte(dev, (data_len & 0xff00) >> 8);

	/* Send the payload */
	for (frag = pkt->frags; frag; frag = frag->frags) {
		for (i = 0U; i < frag->len; ++i) {
			eth_stellaris_send_byte(dev, frag->data[i]);
		}
	}

	/* Will transmit the partial word. */
	eth_stellaris_flush(dev);

	/* Enable transmit. */
	sys_write32(BIT_MACTR_NEWTX, REG_MACTR);

	/* Wait and check if transmit successful or not. */
	k_sem_take(&dev_data->tx_sem, K_FOREVER);

	if (dev_data->tx_err) {
		dev_data->tx_err = false;
		return -EIO;
	}

	LOG_DBG("pkt sent %p len %d", pkt, data_len);

	return 0;
}

static void eth_stellaris_rx_error(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	uint32_t val;

	eth_stats_update_errors_rx(iface);

	/* Clear the rx_frame buffer,
	 * otherwise it could lead to underflow errors
	 */
	sys_write32(0x0, REG_MACRCTL);
	sys_write32(BIT_MACRCTL_RSTFIFO, REG_MACRCTL);
	val = BIT_MACRCTL_BADCRC | BIT_MACRCTL_RXEN;
	sys_write32(val, REG_MACRCTL);
}

static struct net_pkt *eth_stellaris_rx_pkt(const struct device *dev,
					    struct net_if *iface)
{
	int frame_len, bytes_left;
	struct net_pkt *pkt;
	uint32_t reg_val;
	uint16_t count;
	uint8_t *data;

	/*
	 * The Ethernet frame received from the hardware has the
	 * following format. The first two bytes contains the ethernet
	 * frame length, followed by the actual ethernet frame.
	 *
	 * +---------+---- ... -------+
	 * | Length  | Ethernet Frame |
	 * +---------+---- ... -------+
	 */

	/*
	 * The first word contains the frame length and a portion of
	 * the ethernet frame. Extract the frame length.
	 */
	reg_val = sys_read32(REG_MACDATA);
	frame_len = reg_val & 0x0000ffff;

	pkt = net_pkt_rx_alloc_with_buffer(iface, frame_len,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		return NULL;
	}

	/*
	 * The remaining 2 bytes, in the first word is appended to the
	 * ethernet frame.
	 */
	count = 2U;
	data = (uint8_t *)&reg_val + 2;
	if (net_pkt_write(pkt, data, count)) {
		goto error;
	}

	/* A word has been read already, thus minus 4 bytes to be read. */
	bytes_left = frame_len - 4;

	/* Read the rest of words, minus the partial word and FCS byte. */
	for (; bytes_left > 7; bytes_left -= 4) {
		reg_val = sys_read32(REG_MACDATA);
		count = 4U;
		data = (uint8_t *)&reg_val;
		if (net_pkt_write(pkt, data, count)) {
			goto error;
		}
	}

	/* Handle the last partial word and discard the 4 Byte FCS. */
	while (bytes_left > 0) {
		/* Read the partial word. */
		reg_val = sys_read32(REG_MACDATA);

		/* Discard the last FCS word. */
		if (bytes_left <= 4) {
			bytes_left = 0;
			break;
		}

		count = bytes_left - 4;
		data = (uint8_t *)&reg_val;
		if (net_pkt_write(pkt, data, count)) {
			goto error;
		}

		bytes_left -= 4;
	}

	return pkt;

error:
	net_pkt_unref(pkt);

	return NULL;
}

static void eth_stellaris_rx(const struct device *dev)
{
	struct eth_stellaris_runtime *dev_data = DEV_DATA(dev);
	struct net_if *iface = dev_data->iface;
	struct net_pkt *pkt;

	pkt = eth_stellaris_rx_pkt(dev, iface);
	if (!pkt) {
		LOG_ERR("Failed to read data");
		goto err_mem;
	}

	if (net_recv_data(iface, pkt) < 0) {
		LOG_ERR("Failed to place frame in RX Queue");
		goto pkt_unref;
	}

	return;

pkt_unref:
	net_pkt_unref(pkt);

err_mem:
	eth_stellaris_rx_error(iface);
}

static void eth_stellaris_isr(const struct device *dev)
{
	struct eth_stellaris_runtime *dev_data = DEV_DATA(dev);
	int isr_val = sys_read32(REG_MACRIS);
	uint32_t lock;

	lock = irq_lock();

	/* Acknowledge the interrupt. */
	sys_write32(isr_val, REG_MACRIS);

	if (isr_val & BIT_MACRIS_RXINT) {
		eth_stellaris_rx(dev);
	}

	if (isr_val & BIT_MACRIS_TXEMP) {
		dev_data->tx_err = false;
		k_sem_give(&dev_data->tx_sem);
	}

	if (isr_val & BIT_MACRIS_TXER) {
		LOG_ERR("Transmit Frame Error");
		eth_stats_update_errors_tx(dev_data->iface);
		dev_data->tx_err = true;
		k_sem_give(&dev_data->tx_sem);
	}

	if (isr_val & BIT_MACRIS_RXER) {
		LOG_ERR("Error Receiving Frame");
		eth_stellaris_rx_error(dev_data->iface);
	}

	if (isr_val & BIT_MACRIS_FOV) {
		LOG_ERR("Error Rx Overrun");
		eth_stellaris_rx_error(dev_data->iface);
	}

	irq_unlock(lock);
}

static void eth_stellaris_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_stellaris_config *dev_conf = DEV_CFG(dev);
	struct eth_stellaris_runtime *dev_data = DEV_DATA(dev);

	dev_data->iface = iface;

	/* Assign link local address. */
	net_if_set_link_addr(iface,
			     dev_data->mac_addr, 6, NET_LINK_ETHERNET);

	ethernet_init(iface);

	/* Initialize semaphore. */
	k_sem_init(&dev_data->tx_sem, 0, 1);

	/* Initialize Interrupts. */
	dev_conf->config_func(dev);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_stellaris_stats(const struct device *dev)
{
	return &(DEV_DATA(dev)->stats);
}
#endif

static int eth_stellaris_dev_init(const struct device *dev)
{
	uint32_t value;

	/* Assign MAC address to Hardware */
	eth_stellaris_assign_mac(dev);

	/* Program MCRCTL to clear RXFIFO */
	value = BIT_MACRCTL_RSTFIFO;
	sys_write32(value, REG_MACRCTL);

	/* Enable transmitter */
	value = BIT_MACTCTL_DUPLEX | BIT_MACTCTL_CRC |
		BIT_MACTCTL_PADEN | BIT_MACTCTL_TXEN;
	sys_write32(value, REG_MACTCTL);

	/* Enable Receiver */
	value = BIT_MACRCTL_BADCRC | BIT_MACRCTL_RXEN;
	sys_write32(value, REG_MACRCTL);

	return 0;
}

DEVICE_DECLARE(eth_stellaris);

static void eth_stellaris_irq_config(const struct device *dev)
{
	/* Enable Interrupt. */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    eth_stellaris_isr, DEVICE_GET(eth_stellaris), 0);
	irq_enable(DT_INST_IRQN(0));
}

struct eth_stellaris_config eth_cfg = {
	.mac_base = DT_INST_REG_ADDR(0),
	.config_func = eth_stellaris_irq_config,
};

struct eth_stellaris_runtime eth_data = {
	.mac_addr = DT_INST_PROP(0, local_mac_address),
	.tx_err = false,
	.tx_word = 0,
	.tx_pos = 0,
};

static const struct ethernet_api eth_stellaris_apis = {
	.iface_api.init	= eth_stellaris_init,
	.send =  eth_stellaris_send,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_stellaris_stats,
#endif
};

NET_DEVICE_INIT(eth_stellaris, DT_INST_LABEL(0),
		eth_stellaris_dev_init, device_pm_control_nop,
		&eth_data, &eth_cfg, CONFIG_ETH_INIT_PRIORITY,
		&eth_stellaris_apis, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
