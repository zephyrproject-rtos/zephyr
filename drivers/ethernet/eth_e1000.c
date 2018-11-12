/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME eth_e1000
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr.h>
#include <net/ethernet.h>
#include <pci/pci.h>
#include "eth_e1000_priv.h"

static const char *e1000_reg_to_string(enum e1000_reg_t r)
{
#define _(_x)	case _x: return #_x
	switch (r) {
	_(CTRL);
	_(ICR);
	_(ICS);
	_(IMS);
	_(RCTL);
	_(TCTL);
	_(RDBAL);
	_(RDBAH);
	_(RDLEN);
	_(RDH);
	_(RDT);
	_(TDBAL);
	_(TDBAH);
	_(TDLEN);
	_(TDH);
	_(TDT);
	_(RAL);
	_(RAH);
	}
#undef _
	LOG_ERR("Unsupported register: 0x%x", r);
	k_oops();
	return NULL;
}

static enum ethernet_hw_caps e1000_caps(struct device *dev)
{
	return  ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T | \
		ETHERNET_LINK_1000BASE_T;
}

static size_t e1000_linearize(struct net_pkt *pkt, void *buf, size_t bufsize)
{
	size_t len = net_pkt_ll_reserve(pkt) + pkt->frags->len;
	struct net_buf *nb;

	/* First fragment contains link layer (Ethernet) header */
	memcpy(buf, net_pkt_ll(pkt), len);

	for (nb = pkt->frags->frags; nb; nb = nb->frags) {
		memcpy((u8_t *) buf + len, nb->data, nb->len);
		len += nb->len;
	}

	return len;
}

static int e1000_tx(struct e1000_dev *dev, void *data, size_t data_len)
{
	dev->tx.addr = POINTER_TO_INT(data);
	dev->tx.len = data_len;
	dev->tx.cmd = TDESC_EOP | TDESC_RS;

	iow32(dev, TDT, 1);

	while (!(dev->tx.sta)) {
		k_yield();
	}

	LOG_DBG("tx.sta: 0x%02hx", dev->tx.sta);

	return (dev->tx.sta & TDESC_STA_DD) ? 0 : -EIO;
}

static int e1000_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct e1000_dev *dev = net_if_get_device(iface)->driver_data;

	size_t len = e1000_linearize(pkt, dev->txb, sizeof(dev->txb));

	int err = e1000_tx(dev, dev->txb, len);

	if (err) {
		net_pkt_unref(pkt);
	}

	return err;
}

static struct net_pkt *e1000_rx(struct e1000_dev *dev)
{
	struct net_pkt *pkt = NULL;

	LOG_DBG("rx.sta: 0x%02hx", dev->rx.sta);

	if (!(dev->rx.sta & RDESC_STA_DD)) {
		LOG_ERR("RX descriptor not ready");
		goto out;
	}

	pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Out of RX buffers");
		goto out;
	}

	if (!net_pkt_append_all(pkt, dev->rx.len - 4,
				INT_TO_POINTER((u32_t) dev->rx.addr),
				K_NO_WAIT)) {
		LOG_ERR("Out of memory for received frame");
		net_pkt_unref(pkt);
		pkt = NULL;
	}
out:
	return pkt;
}

static void e1000_isr(struct device *device)
{
	struct e1000_dev *dev = device->driver_data;
	u32_t icr = ior32(dev, ICR); /* Cleared upon read */

	icr &= ~(ICR_TXDW | ICR_TXQE);

	if (icr & ICR_RXO) {
		struct net_pkt *pkt = e1000_rx(dev);

		icr &= ~ICR_RXO;

		if (pkt) {
			net_recv_data(dev->iface, pkt);
		}
	}

	if (icr) {
		LOG_ERR("Unhandled interrupt, ICR: 0x%x", icr);
	}
}

int e1000_probe(struct device *device)
{
	struct e1000_dev *dev = device->driver_data;

	pci_bus_scan_init();

	if (pci_bus_scan(&dev->pci)) {

		pci_enable_regs(&dev->pci);

		pci_enable_bus_master(&dev->pci);

		pci_show(&dev->pci);

		return 0;
	}

	return -ENODEV;
}

static struct device DEVICE_NAME_GET(eth_e1000);

static void e1000_init(struct net_if *iface)
{
	struct e1000_dev *dev = net_if_get_device(iface)->driver_data;
	u32_t ral, rah;

	dev->iface = iface;

	/* Setup TX descriptor */

	iow32(dev, TDBAL, (u32_t) &dev->tx);
	iow32(dev, TDBAH, 0);
	iow32(dev, TDLEN, 1*16);

	iow32(dev, TDH, 0);
	iow32(dev, TDT, 0);

	iow32(dev, TCTL, TCTL_EN);

	/* Setup RX descriptor */

	dev->rx.addr = POINTER_TO_INT(dev->rxb);
	dev->rx.len = sizeof(dev->rxb);

	iow32(dev, RDBAL, (u32_t) &dev->rx);
	iow32(dev, RDBAH, 0);
	iow32(dev, RDLEN, 1*16);

	iow32(dev, RDH, 0);
	iow32(dev, RDT, 1);

	iow32(dev, IMS, IMS_RXO);

	ral = ior32(dev, RAL);
	rah = ior32(dev, RAH);

	memcpy(dev->mac, &ral, 4);
	memcpy(dev->mac + 4, &rah, 2);

	ethernet_init(iface);

	net_if_set_link_addr(iface, dev->mac, sizeof(dev->mac),
				NET_LINK_ETHERNET);

	IRQ_CONNECT(CONFIG_ETH_E1000_IRQ, CONFIG_ETH_E1000_IRQ_PRIORITY,
			e1000_isr, DEVICE_GET(eth_e1000),
			CONFIG_ETH_E1000_IRQ_FLAGS);

	irq_enable(CONFIG_ETH_E1000_IRQ);

	iow32(dev, CTRL, CTRL_SLU); /* Set link up */

	iow32(dev, RCTL, RCTL_EN | RCTL_MPE);

	LOG_DBG("done");
}

#define PCI_VENDOR_ID_INTEL	0x8086
#define PCI_DEVICE_ID_I82540EM	0x100e

static struct e1000_dev e1000_dev = {
	.pci.vendor_id = PCI_VENDOR_ID_INTEL,
	.pci.device_id = PCI_DEVICE_ID_I82540EM,
};

static const struct ethernet_api e1000_api = {
	.iface_api.init		= e1000_init,
	.iface_api.send		= e1000_send,
	.get_capabilities	= e1000_caps,
};

NET_DEVICE_INIT(eth_e1000,
		"ETH_0",
		e1000_probe,
		&e1000_dev,
		NULL,
		CONFIG_ETH_INIT_PRIORITY,
		&e1000_api,
		ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2),
		E1000_MTU);
