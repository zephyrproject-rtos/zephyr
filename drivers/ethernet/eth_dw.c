/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <sys_io.h>
#include <board.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "ethernet.h"
#include "eth_dw_priv.h"
#include <net/ip/net_driver_ethernet.h>
#include <misc/__assert.h>

#ifdef CONFIG_SHARED_IRQ
#include <shared_irq.h>
#endif

static inline uint32_t eth_read(uint32_t base_addr, uint32_t offset)
{
	return sys_read32(base_addr + offset);
}

static inline void eth_write(uint32_t base_addr, uint32_t offset,
			     uint32_t val)
{
	sys_write32(val, base_addr + offset);
}

static void eth_rx(struct device *port)
{
	struct eth_runtime *context = port->driver_data;
	struct eth_config *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;
	struct net_buf *buf;
	uint32_t frm_len = 0;

	/* Check whether the RX descriptor is still owned by the device.  If not,
	 * process the received frame or an error that may have occurred.
	 */
	if (context->rx_desc.own == 1) {
		ETH_ERR("Spurious receive interrupt from Ethernet MAC.\n");
		return;
	}

	if (!net_driver_ethernet_is_opened()) {
		goto release_desc;
	}

	if (context->rx_desc.err_summary) {
		ETH_ERR("Error receiving frame: RDES0 = %08x, RDES1 = %08x.\n",
			context->rx_desc.rdes0, context->rx_desc.rdes1);
		goto release_desc;
	}

	buf = ip_buf_get_reserve_rx(0);
	if (buf == NULL) {
		ETH_ERR("Failed to obtain RX buffer.\n");
		goto release_desc;
	}

	frm_len = context->rx_desc.frm_len;
	if (frm_len > UIP_BUFSIZE) {
		ETH_ERR("Frame too large: %u.\n", frm_len);
		goto release_desc;
	}

	memcpy(net_buf_add(buf, frm_len), (void *)context->rx_buf, frm_len);
	uip_len(buf) = frm_len;

	net_driver_ethernet_recv(buf);

release_desc:
	/* Return ownership of the RX descriptor to the device. */
	context->rx_desc.own = 1;

	/* Request that the device check for an available RX descriptor, since
	 * ownership of the descriptor was just transferred to the device.
	 */
	eth_write(base_addr, REG_ADDR_RX_POLL_DEMAND, 1);
}

/* @brief Transmit the current Ethernet frame.
 *
 *        This procedure will block indefinitely until the Ethernet device is
 *        ready to accept a new outgoing frame.  It then copies the current
 *        Ethernet frame from the global uip_buf buffer to the device DMA
 *        buffer and signals to the device that a new frame is available to be
 *        transmitted.
 */
static int eth_tx(struct device *port, struct net_buf *buf)
{
	struct eth_runtime *context = port->driver_data;
	struct eth_config *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;

	/* Wait until the TX descriptor is no longer owned by the device. */
	while (context->tx_desc.own == 1) {
	}

#ifdef CONFIG_ETHERNET_DEBUG
	/* Check whether an error occurred transmitting the previous frame. */
	if (context->tx_desc.err_summary) {
		ETH_ERR("Error transmitting frame: TDES0 = %08x, TDES1 = %08x.\n",
			context->tx_desc.tdes0, context->tx_desc.tdes1);
	}
#endif

	/* Transmit the next frame. */
	if (uip_len(buf) > UIP_BUFSIZE) {
		ETH_ERR("Frame too large to TX: %u\n", uip_len(buf));

		return -1;
	}

	memcpy((void *)context->tx_buf, uip_buf(buf), uip_len(buf));

	context->tx_desc.tx_buf1_sz = uip_len(buf);

	context->tx_desc.own = 1;

	/* Request that the device check for an available TX descriptor, since
	 * ownership of the descriptor was just transferred to the device.
	 */
	eth_write(base_addr, REG_ADDR_TX_POLL_DEMAND, 1);

	return 1;
}

void eth_dw_isr(struct device *port)
{
	struct eth_config *config = port->config->config_info;
	uint32_t base_addr = config->base_addr;
	uint32_t int_status;

	int_status = eth_read(base_addr, REG_ADDR_STATUS);

#ifdef CONFIG_SHARED_IRQ
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
	eth_write(base_addr, REG_ADDR_STATUS, STATUS_RX_INT);
}

#ifdef CONFIG_PCI
static inline int eth_setup(struct device *dev)
{
	struct eth_config *config = dev->config->config_info;

	pci_bus_scan_init();

	if (!pci_bus_scan(&config->pci_dev))
		return 0;

#ifdef CONFIG_PCI_ENUMERATION
	config->base_addr = config->pci_dev.addr;
	config->irq_num = config->pci_dev.irq;
#endif
	pci_enable_regs(&config->pci_dev);
	pci_enable_bus_master(&config->pci_dev);

	pci_show(&config->pci_dev);

	return 1;
}
#else
#define eth_setup(_unused_) (1)
#endif /* CONFIG_PCI */

static int eth_net_tx(struct net_buf *buf);

static int eth_initialize(struct device *port)
{
	struct eth_runtime *context = port->driver_data;
	struct eth_config *config = port->config->config_info;
	uint32_t base_addr;

	union {
		struct {
			uint8_t bytes[6];
			uint8_t pad[2];
		} __attribute__((packed));
		uint32_t words[2];
	} mac_addr;

	if (!eth_setup(port))
		return DEV_NOT_CONFIG;

	base_addr = config->base_addr;

	/* Read the MAC address from the device. */
	mac_addr.words[1] = eth_read(base_addr, REG_ADDR_MACADDR_HI);
	mac_addr.words[0] = eth_read(base_addr, REG_ADDR_MACADDR_LO);

	net_set_mac(mac_addr.bytes, sizeof(mac_addr.bytes));

	/* Initialize transmit descriptor. */
	context->tx_desc.tdes0 = 0;
	context->tx_desc.tdes1 = 0;

	context->tx_desc.buf1_ptr = (uint8_t *)context->tx_buf;
	context->tx_desc.tx_end_of_ring = 1;
	context->tx_desc.first_seg_in_frm = 1;
	context->tx_desc.last_seg_in_frm = 1;
	context->tx_desc.tx_end_of_ring = 1;

	/* Initialize receive descriptor. */
	context->rx_desc.rdes0 = 0;
	context->rx_desc.rdes1 = 0;

	context->rx_desc.buf1_ptr = (uint8_t *)context->rx_buf;
	context->rx_desc.own = 1;
	context->rx_desc.first_desc = 1;
	context->rx_desc.last_desc = 1;
	context->rx_desc.rx_buf1_sz = UIP_BUFSIZE;
	context->rx_desc.rx_end_of_ring = 1;

	/* Install transmit and receive descriptors. */
	eth_write(base_addr, REG_ADDR_RX_DESC_LIST, (uint32_t)&context->rx_desc);
	eth_write(base_addr, REG_ADDR_TX_DESC_LIST, (uint32_t)&context->tx_desc);

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

	eth_write(base_addr, REG_ADDR_DMA_OPERATION,
		  /* Enable receive store-and-forward mode for simplicity. */
		  OP_MODE_25_RX_STORE_N_FORWARD |
		  /* Enable transmit store-and-forward mode for simplicity. */
		  OP_MODE_21_TX_STORE_N_FORWARD |
		  /* Place the transmitter state machine in the Running state. */
		  OP_MODE_13_START_TX |
		  /* Place the receiver state machine in the Running state. */
		  OP_MODE_1_START_RX);

	ETH_INFO("Enabled 100M full-duplex mode.\n");

	net_driver_ethernet_register_tx(eth_net_tx);

	config->config_func(port);

	return 0;
}

/* Bindings to the plaform */
#if CONFIG_ETH_DW_0
static void eth_config_0_irq(struct device *port);

static struct eth_config eth_config_0 = {
	.base_addr		= CONFIG_ETH_DW_0_BASE_ADDR,
#ifdef CONFIG_ETH_DW_0_IRQ_DIRECT
	.irq_num		= CONFIG_ETH_DW_0_IRQ,
#endif
#if CONFIG_PCI
	.pci_dev.class		= CONFIG_ETH_DW_CLASS,
	.pci_dev.bus		= CONFIG_ETH_DW_0_BUS,
	.pci_dev.dev		= CONFIG_ETH_DW_0_DEV,
	.pci_dev.vendor_id	= CONFIG_ETH_DW_VENDOR_ID,
	.pci_dev.device_id	= CONFIG_ETH_DW_DEVICE_ID,
	.pci_dev.function	= CONFIG_ETH_DW_0_FUNCTION,
	.pci_dev.bar		= CONFIG_ETH_DW_0_BAR,
#endif
	.config_func		= eth_config_0_irq,

#ifdef CONFIG_ETH_DW_0_IRQ_SHARED
	.shared_irq_dev_name	= CONFIG_ETH_DW_0_IRQ_SHARED_NAME,
#endif
};

static struct eth_runtime eth_0_runtime;

DECLARE_DEVICE_INIT_CONFIG(eth_dw_0, CONFIG_ETH_DW_0_NAME,
			   eth_initialize, &eth_config_0);
SYS_DEFINE_DEVICE(eth_dw_0, &eth_0_runtime, NANOKERNEL,
					CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

static int eth_net_tx(struct net_buf *buf)
{
	return eth_tx(&__initconfig_eth_dw_0, buf);
}

#ifdef CONFIG_ETH_DW_0_IRQ_DIRECT
IRQ_CONNECT_STATIC(eth_dw_0, CONFIG_ETH_DW_0_IRQ,
		   CONFIG_ETH_DW_0_PRI, eth_dw_isr, 0);
#endif

static void eth_config_0_irq(struct device *port)
{
	struct eth_config *config = port->config->config_info;
	struct device *shared_irq_dev;

#ifdef CONFIG_ETH_DW_0_IRQ_DIRECT
	ARG_UNUSED(shared_irq_dev);
	IRQ_CONFIG(eth_dw_0, config->irq_num);
	irq_enable(config->irq_num);
#elif defined(CONFIG_ETH_DW_0_IRQ_SHARED)
	shared_irq_dev = device_get_binding(config->shared_irq_dev_name);
	__ASSERT(shared_irq_dev != NULL, "Failed to get eth_dw device binding");
	shared_irq_isr_register(shared_irq_dev, (isr_t)eth_dw_isr, port);
	shared_irq_enable(shared_irq_dev, port);
#endif
}

#ifdef CONFIG_ETH_DW_0_IRQ_DIRECT
struct device *eth_dw_isr_0 = SYS_GET_DEVICE(eth_dw_0);
#endif  /* CONFIG_ETH_DW_0_IRQ_DIRECT */

#endif  /* CONFIG_ETH_DW_0 */
