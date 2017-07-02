/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define SYS_LOG_DOMAIN "ETH DW"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ETHERNET_LEVEL
#include <logging/sys_log.h>

#include <board.h>
#include <device.h>
#include <errno.h>
#include <init.h>
#include <kernel.h>
#include <misc/__assert.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys_io.h>

#include "eth_dw_priv.h"

#ifdef CONFIG_SHARED_IRQ
#include <shared_irq.h>
#endif

#define TX_BUSY_LOOP_SPINS 20

static inline u32_t eth_read(u32_t base_addr, u32_t offset)
{
	return sys_read32(base_addr + offset);
}

static inline void eth_write(u32_t base_addr, u32_t offset,
			     u32_t val)
{
	sys_write32(val, base_addr + offset);
}

static void eth_rx(struct device *port)
{
	struct eth_runtime *context = port->driver_data;
	struct net_pkt *pkt;
	u32_t frm_len;
	int r;

	/* Check whether the RX descriptor is still owned by the device.  If not,
	 * process the received frame or an error that may have occurred.
	 */
	if (context->rx_desc.own) {
		SYS_LOG_ERR("Spurious receive interrupt from Ethernet MAC");
		return;
	}

	if (context->rx_desc.err_summary) {
		SYS_LOG_ERR("Error receiving frame: RDES0 = %08x, RDES1 = %08x",
			context->rx_desc.rdes0, context->rx_desc.rdes1);
		goto release_desc;
	}

	frm_len = context->rx_desc.frm_len;
	if (frm_len > sizeof(context->rx_buf)) {
		SYS_LOG_ERR("Frame too large: %u", frm_len);
		goto release_desc;
	}

	/* Throw away the last 4 bytes (CRC).  See Intel® Quark TM SoC X1000
	 * datasheet, Table 95 (Receive Descriptor Fields (RDES0)), "frame
	 * length":
	 *    These bits indicate the byte length of the received frame that
	 *    was transferred to host memory (including CRC).
	 * If the CRC is not removed here, packet processing in upper layers
	 * will fail since the packet length will be different from the
	 * received frame length by exactly 4 bytes.
	 */
	if (frm_len < sizeof(u32_t)) {
		SYS_LOG_ERR("Frame too small: %u", frm_len);
		goto release_desc;
	} else {
		frm_len -= sizeof(u32_t);
	}

	pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!pkt) {
		SYS_LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	if (!net_pkt_append_all(pkt, frm_len, (u8_t *)context->rx_buf,
				K_NO_WAIT)) {
		SYS_LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		goto release_desc;
	}

	r = net_recv_data(context->iface, pkt);
	if (r < 0) {
		SYS_LOG_ERR("Failed to enqueue frame into RX queue: %d", r);
		net_pkt_unref(pkt);
	}

release_desc:
	/* Return ownership of the RX descriptor to the device. */
	context->rx_desc.own = 1;

	/* Request that the device check for an available RX descriptor, since
	 * ownership of the descriptor was just transferred to the device.
	 */
	eth_write(context->base_addr, REG_ADDR_RX_POLL_DEMAND, 1);
}

static void eth_tx_spin_wait(struct eth_runtime *context)
{
	int spins;

	for (spins = 0; spins < TX_BUSY_LOOP_SPINS; spins++) {
		if (!context->tx_desc.own) {
			return;
		}
	}

	while (context->tx_desc.own) {
		k_yield();
	}
}

static void eth_tx_data(struct eth_runtime *context, u8_t *data, u16_t len)
{
#ifdef CONFIG_NET_DEBUG_L2_ETHERNET
	/* Check whether an error occurred transmitting the previous frame. */
	if (context->tx_desc.err_summary) {
		SYS_LOG_ERR("Error transmitting frame: TDES0 = %08x,"
			    "TDES1 = %08x", context->tx_desc.tdes0,
			    context->tx_desc.tdes1);
	}
#endif

	/* Update transmit descriptor. */
	context->tx_desc.buf1_ptr = data;
	context->tx_desc.tx_buf1_sz = len;
	eth_write(context->base_addr, REG_ADDR_TX_DESC_LIST,
		  (u32_t)&context->tx_desc);

	context->tx_desc.own = 1;

	/* Request that the device check for an available TX descriptor, since
	 * ownership of the descriptor was just transferred to the device.
	 */
	eth_write(context->base_addr, REG_ADDR_TX_POLL_DEMAND, 1);

	/* Ensure DMA transfer has been completed. */
	eth_tx_spin_wait(context);
}

/* @brief Transmit the current Ethernet frame.
 *
 *	This procedure will block indefinitely until all fragments from a
 *	net_buf have been transmitted.  Data is copied using DMA directly
 *	from each fragment's data pointer.  This procedure might yield to
 *	other threads  while waiting for the DMA transfer to finish.
 */
static int eth_tx(struct net_if *iface, struct net_pkt *pkt)
{
	struct device *port = net_if_get_device(iface);
	struct eth_runtime *context = port->driver_data;

	/* Ensure we're clear to transmit. */
	eth_tx_spin_wait(context);

	if (!pkt->frags) {
		eth_tx_data(context, net_pkt_ll(pkt),
			    net_pkt_ll_reserve(pkt));
	} else {
		struct net_buf *frag;

		eth_tx_data(context, net_pkt_ll(pkt),
			    net_pkt_ll_reserve(pkt) + pkt->frags->len);
		for (frag = pkt->frags->frags; frag; frag = frag->frags) {
			eth_tx_data(context, frag->data, frag->len);
		}
	}

	net_pkt_unref(pkt);
	return 0;
}

static void eth_dw_isr(struct device *port)
{
	struct eth_runtime *context = port->driver_data;
#ifdef CONFIG_SHARED_IRQ
	u32_t int_status;

	int_status = eth_read(context->base_addr, REG_ADDR_STATUS);

	/* If using with shared IRQ, this function will be called
	 * by the shared IRQ driver. So check here if the interrupt
	 * is coming from the GPIO controller (or somewhere else).
	 */
	if ((int_status & STATUS_RX_INT) == 0) {
		return;
	}
#endif
	eth_rx(port);

	/* Acknowledge the interrupt. */
	eth_write(context->base_addr, REG_ADDR_STATUS,
		  STATUS_NORMAL_INT | STATUS_RX_INT);
}

#ifdef CONFIG_PCI
static inline int eth_setup(struct device *dev)
{
	struct eth_runtime *context = dev->driver_data;

	pci_bus_scan_init();

	if (!pci_bus_scan(&context->pci_dev))
		return 0;

#ifdef CONFIG_PCI_ENUMERATION
	context->base_addr = context->pci_dev.addr;
#endif
	pci_enable_regs(&context->pci_dev);
	pci_enable_bus_master(&context->pci_dev);

	pci_show(&context->pci_dev);

	return 1;
}
#else
#define eth_setup(_unused_) (1)
#endif /* CONFIG_PCI */

static int eth_initialize_internal(struct net_if *iface)
{
	struct device *port = net_if_get_device(iface);
	struct eth_runtime *context = port->driver_data;
	const struct eth_config *config = port->config->config_info;
	u32_t base_addr;

	context->iface = iface;
	base_addr = context->base_addr;

	/* Read the MAC address from the device. */
	context->mac_addr.words[1] = eth_read(base_addr, REG_ADDR_MACADDR_HI);
	context->mac_addr.words[0] = eth_read(base_addr, REG_ADDR_MACADDR_LO);

	net_if_set_link_addr(context->iface, context->mac_addr.bytes,
			     sizeof(context->mac_addr.bytes),
			     NET_LINK_ETHERNET);

	/* Initialize the frame filter enabling unicast messages */
	eth_write(base_addr, REG_ADDR_MAC_FRAME_FILTER, MAC_FILTER_4_PM);


	/* Initialize receive descriptor. */
	context->rx_desc.rdes0 = 0;
	context->rx_desc.rdes1 = 0;

	context->rx_desc.buf1_ptr = (u8_t *)context->rx_buf;
	context->rx_desc.first_desc = 1;
	context->rx_desc.last_desc = 1;
	context->rx_desc.own = 1;
	context->rx_desc.rx_buf1_sz = sizeof(context->rx_buf);
	context->rx_desc.rx_end_of_ring = 1;

	/* Install receive descriptor. */
	eth_write(base_addr, REG_ADDR_RX_DESC_LIST, (u32_t)&context->rx_desc);

	/* Initialize transmit descriptor. */
	context->tx_desc.tdes0 = 0;
	context->tx_desc.tdes1 = 0;
	context->tx_desc.buf1_ptr = NULL;
	context->tx_desc.tx_buf1_sz = 0;
	context->tx_desc.first_seg_in_frm = 1;
	context->tx_desc.last_seg_in_frm = 1;
	context->tx_desc.tx_end_of_ring = 1;

	/* Install transmit descriptor. */
	eth_write(context->base_addr, REG_ADDR_TX_DESC_LIST,
		  (u32_t)&context->tx_desc);

	eth_write(base_addr, REG_ADDR_MAC_CONF,
		  /* Set the RMII speed to 100Mbps */
		  MAC_CONF_14_RMII_100M |
		  /* Enable full-duplex mode */
		  MAC_CONF_11_DUPLEX |
		  /* Enable transmitter */
		  MAC_CONF_3_TX_EN |
		  /* Enable receiver */
		  MAC_CONF_2_RX_EN);

	eth_write(base_addr, REG_ADDR_INT_ENABLE,
		  INT_ENABLE_NORMAL |
		  /* Enable receive interrupts */
		  INT_ENABLE_RX);

	/* Mask all the MMC interrupts */
	eth_write(base_addr, REG_MMC_RX_INTR_MASK, MMC_DEFAULT_MASK);
	eth_write(base_addr, REG_MMC_TX_INTR_MASK, MMC_DEFAULT_MASK);
	eth_write(base_addr, REG_MMC_RX_IPC_INTR_MASK, MMC_DEFAULT_MASK);

	eth_write(base_addr, REG_ADDR_DMA_OPERATION,
		  /* Enable receive store-and-forward mode for simplicity. */
		  OP_MODE_25_RX_STORE_N_FORWARD |
		  /* Enable transmit store-and-forward mode for simplicity. */
		  OP_MODE_21_TX_STORE_N_FORWARD |
		  /* Place the transmitter state machine in the Running state. */
		  OP_MODE_13_START_TX |
		  /* Place the receiver state machine in the Running state. */
		  OP_MODE_1_START_RX);

	SYS_LOG_INF("Enabled 100M full-duplex mode");

	config->config_func(port);

	return 0;
}

static void eth_initialize(struct net_if *iface)
{
	int r = eth_initialize_internal(iface);

	if (r < 0) {
		SYS_LOG_ERR("Could not initialize ethernet device: %d", r);
	}
}

/* Bindings to the plaform */
#if CONFIG_ETH_DW_0
static void eth_config_0_irq(struct device *port)
{
	const struct eth_config *config = port->config->config_info;
	struct device *shared_irq_dev;

#ifdef CONFIG_ETH_DW_0_IRQ_DIRECT
	ARG_UNUSED(shared_irq_dev);
	IRQ_CONNECT(ETH_DW_0_IRQ, CONFIG_ETH_DW_0_IRQ_PRI, eth_dw_isr,
		    DEVICE_GET(eth_dw_0), 0);
	irq_enable(ETH_DW_0_IRQ);
#elif defined(CONFIG_ETH_DW_0_IRQ_SHARED)
	shared_irq_dev = device_get_binding(config->shared_irq_dev_name);
	__ASSERT(shared_irq_dev != NULL, "Failed to get eth_dw device binding");
	shared_irq_isr_register(shared_irq_dev, (isr_t)eth_dw_isr, port);
	shared_irq_enable(shared_irq_dev, port);
#endif
}

static const struct eth_config eth_config_0 = {
#ifdef CONFIG_ETH_DW_0_IRQ_DIRECT
	.irq_num		= ETH_DW_0_IRQ,
#endif
	.config_func		= eth_config_0_irq,

#ifdef CONFIG_ETH_DW_0_IRQ_SHARED
	.shared_irq_dev_name	= CONFIG_ETH_DW_0_IRQ_SHARED_NAME,
#endif
};

static struct eth_runtime eth_0_runtime = {
	.base_addr		= ETH_DW_0_BASE_ADDR,
#if CONFIG_PCI
	.pci_dev.class_type	= ETH_DW_PCI_CLASS,
	.pci_dev.bus		= ETH_DW_0_PCI_BUS,
	.pci_dev.dev		= ETH_DW_0_PCI_DEV,
	.pci_dev.vendor_id	= ETH_DW_PCI_VENDOR_ID,
	.pci_dev.device_id	= ETH_DW_PCI_DEVICE_ID,
	.pci_dev.function	= ETH_DW_0_PCI_FUNCTION,
	.pci_dev.bar		= ETH_DW_0_PCI_BAR,
#endif
};

static struct net_if_api api_funcs = {
	.init = eth_initialize,
	.send = eth_tx,
};

NET_DEVICE_INIT(eth_dw_0, CONFIG_ETH_DW_0_NAME,
		eth_setup, &eth_0_runtime,
		&eth_config_0, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
		ETH_DW_MTU);
#endif  /* CONFIG_ETH_DW_0 */
