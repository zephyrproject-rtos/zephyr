/*
 * Xilinx AXI 1G / 2.5G Ethernet Subsystem
 *
 * Copyright(c) 2024, CISPA Helmholtz Center for Information Security
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_xilinx_axienet, CONFIG_ETHERNET_LOG_LEVEL);

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/net/phy.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>

#include "../dma/dma_xilinx_axi_dma.h"

/* register offsets and masks */
#define XILINX_AXIENET_INTERRUPT_STATUS_OFFSET         0x0000000C
#define XILINX_AXIENET_INTERRUPT_STATUS_RXREJ_MASK     0x00000008
#define XILINX_AXIENET_INTERRUPT_STATUS_RXFIFOOVR_MASK 0x00000010 /* Rx fifo overrun */
#define XILINX_AXIENET_INTERRUPT_PENDING_OFFSET        0x00000010

#define XILINX_AXIENET_INTERRUPT_PENDING_RXCMPIT_MASK     0x00000004 /* Rx complete */
#define XILINX_AXIENET_INTERRUPT_PENDING_RXRJECT_MASK     0x00000008 /* Rx frame rejected */
#define XILINX_AXIENET_INTERRUPT_PENDING_RXFIFOOVR_MASK   0x00000010 /* Rx fifo overrun */
#define XILINX_AXIENET_INTERRUPT_PENDING_TXCMPIT_MASK     0x00000020 /* Tx complete */
#define XILINX_AXIENET_INTERRUPT_PENDING_RXDCMLOCK_MASK   0x00000040 /* Rx Dcm Lock */
#define XILINX_AXIENET_INTERRUPT_PENDING_MGTRDY_MASK      0x00000080 /* MGT clock Lock */
#define XILINX_AXIENET_INTERRUPT_PENDING_PHYRSTCMPLT_MASK 0x00000100 /* Phy Reset complete */

#define XILINX_AXIENET_INTERRUPT_ENABLE_OFFSET      0x00000014
#define XILINX_AXIENET_INTERRUPT_ENABLE_RXREJ_MASK  0x00000008
#define XILINX_AXIENET_INTERRUPT_ENABLE_OVR_MASK    0x00000010 /* FIFO overrun */

#define XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_0_REG_OFFSET     0x00000400
#define XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET     0x00000404
#define XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_RX_EN_MASK 0x10000000
#define XILINX_AXIENET_RECEIVER_CONFIGURATION_FLOW_CONTROL_OFFSET   0x0000040C
#define XILINX_AXIENET_RECEIVER_CONFIGURATION_FLOW_CONTROL_EN_MASK  0x20000000
#define XILINX_AXIENET_TX_CONTROL_REG_OFFSET                        0x00000408
#define XILINX_AXIENET_TX_CONTROL_TX_EN_MASK                        (1 << 11)

#define XILINX_AXIENET_UNICAST_ADDRESS_WORD_0_OFFSET 0x00000700
#define XILINX_AXIENET_UNICAST_ADDRESS_WORD_1_OFFSET 0x00000704

/* device state */
struct xilinx_axienet_data {
	/* device mac address */
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	uint8_t tx_buffer[NET_ETH_MAX_FRAME_SIZE];
	uint8_t rx_buffer[NET_ETH_MAX_FRAME_SIZE];

	struct net_if *interface;

	bool dma_is_configured_rx, dma_is_configured_tx;
};

/* global configuration per Ethernet device */
struct xilinx_axienet_config {
	void (*config_func)(const struct xilinx_axienet_data *dev);
	const struct device *dma;

	const struct device *phy;

	void *reg;

	bool have_irq, have_rx_csum_offload, have_tx_csum_offload;
};

static void xilinx_axienet_write_register(const struct xilinx_axienet_config *config,
					  uintptr_t reg_offset, uint32_t value)
{
	volatile uint32_t *reg_addr = (uint32_t *)((uint8_t *)(config->reg) + reg_offset);

	*reg_addr = value;
	barrier_dmem_fence_full(); /* make sure that write commits */
}

static uint32_t xilinx_axienet_read_register(const struct xilinx_axienet_config *config,
					     uintptr_t reg_offset)
{
	const volatile uint32_t *reg_addr = (uint32_t *)((uint8_t *)(config->reg) + reg_offset);
	const uint32_t ret = *reg_addr;

	barrier_dmem_fence_full(); /* make sure that read commits */
	return ret;
}
static int setup_dma_rx_transfer(const struct device *dev,
				 const struct xilinx_axienet_config *config,
				 struct xilinx_axienet_data *data);

/* called by DMA when a packet is available */
static void xilinx_axienet_rx_callback(const struct device *dma, void *user_data, uint32_t channel,
				       int status)
{
	struct device *ethdev = (struct device *)user_data;
	struct xilinx_axienet_data *data = ethdev->data;
	unsigned int packet_size;
	struct net_pkt *pkt;

	if (status < 0) {
		LOG_ERR("DMA RX error: %d", status);
		eth_stats_update_errors_rx(data->interface);
		goto setup_new_transfer;
	}

	packet_size = dma_xilinx_axi_dma_last_received_frame_length(dma);
	pkt = net_pkt_rx_alloc_with_buffer(data->interface, packet_size, AF_UNSPEC, 0, K_NO_WAIT);

	if (!pkt) {
		LOG_ERR("Could not allocate a packet!");
		goto setup_new_transfer;
	}
	if (net_pkt_write(pkt, data->rx_buffer, packet_size)) {
		LOG_ERR("Could not write RX buffer into packet!");
		net_pkt_unref(pkt);
		goto setup_new_transfer;
	}
	if (net_recv_data(data->interface, pkt) < 0) {
		LOG_ERR("Coult not receive packet data!");
		net_pkt_unref(pkt);
		goto setup_new_transfer;
	}

	LOG_DBG("Packet with %u bytes received!\n", packet_size);

	/* we need to start a new DMA transfer irregardless of whether the DMA reported an error */
	/* otherwise, the ethernet subsystem would just stop receiving */
setup_new_transfer:
	if (setup_dma_rx_transfer(ethdev, ethdev->config, ethdev->data)) {
		LOG_ERR("Could not set up next RX DMA transfer!");
	}
}

static void xilinx_axienet_tx_callback(const struct device *dev, void *user_data, uint32_t channel,
				       int status)
{
	struct device *ethdev = (struct device *)user_data;
	struct xilinx_axienet_data *data = ethdev->data;
	/* might not be used, depending on config */
	(void)data;

	if (status < 0) {
		LOG_ERR("DMA TX error: %d", status);
		eth_stats_update_errors_tx(data->interface);
	}
}

static int setup_dma_rx_transfer(const struct device *dev,
				 const struct xilinx_axienet_config *config,
				 struct xilinx_axienet_data *data)
{
	int err;

	if (!data->dma_is_configured_rx) {
		static struct dma_block_config head_block = {0};
		static struct dma_config dma_conf = {0};

		head_block.source_address = 0x0;
		head_block.dest_address = (uintptr_t)data->rx_buffer;
		head_block.block_size = sizeof(data->rx_buffer);
		head_block.next_block = NULL;
		head_block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		head_block.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

		dma_conf.dma_slot = 0;
		dma_conf.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_conf.complete_callback_en = 1;
		dma_conf.error_callback_dis = 0;
		dma_conf.block_count = 1;
		dma_conf.head_block = &head_block;
		dma_conf.user_data = (void *)dev;
		dma_conf.dma_callback = xilinx_axienet_rx_callback;

		if (config->have_rx_csum_offload) {
			dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_FULL_CSUM_OFFLOAD;
		} else {
			dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_NO_CSUM_OFFLOAD;
		}

		if (!config->dma) {
			LOG_ERR("DMA handle is not provided in device tree!");
			return -ENODEV;
		}

		err = dma_config(config->dma, XILINX_AXI_DMA_RX_CHANNEL_NUM, &dma_conf);
		if (err) {
			LOG_ERR("DMA config failed: %d", err);
			return err;
		}

		data->dma_is_configured_rx = true;
	} else {
		/* can use faster "reload" API, as everything else stays the same */
		err = dma_reload(config->dma, XILINX_AXI_DMA_RX_CHANNEL_NUM, 0x0,
				 (uintptr_t)data->rx_buffer, sizeof(data->rx_buffer));
		if (err) {
			LOG_ERR("DMA reconfigure failed: %d", err);
			return err;
		}
	}
	LOG_DBG("Transmitting one packet with DMA!");
	return dma_start(config->dma, XILINX_AXI_DMA_RX_CHANNEL_NUM);
}

/* assumes that the caller has set up data->tx_buffer */
static int setup_dma_tx_transfer(const struct device *dev,
				 const struct xilinx_axienet_config *config,
				 struct xilinx_axienet_data *data, uint32_t buffer_len)
{
	int err;

	if (!data->dma_is_configured_tx) {
		static struct dma_block_config head_block;
		static struct dma_config dma_conf;

		head_block.source_address = (uintptr_t)data->tx_buffer;
		head_block.dest_address = 0x0;
		head_block.block_size = buffer_len;
		head_block.next_block = NULL;
		head_block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		head_block.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;

		dma_conf.dma_slot = 0;
		dma_conf.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_conf.complete_callback_en = 1;
		dma_conf.error_callback_dis = 0;
		dma_conf.block_count = 1;
		dma_conf.head_block = &head_block;
		dma_conf.user_data = (void *)dev;
		dma_conf.dma_callback = xilinx_axienet_tx_callback;

		if (config->have_tx_csum_offload) {
			dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_FULL_CSUM_OFFLOAD;
		} else {
			dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_NO_CSUM_OFFLOAD;
		}

		if (!config->dma) {
			LOG_ERR("DMA handle is not provided in device tree!");
			return -EINVAL;
		}

		err = dma_config(config->dma, XILINX_AXI_DMA_TX_CHANNEL_NUM, &dma_conf);
		if (err) {
			LOG_ERR("DMA config failed: %d", err);
			return err;
		}

		data->dma_is_configured_tx = true;
	} else {
		/* can use faster "reload" API, as everything else stays the same */
		err = dma_reload(config->dma, XILINX_AXI_DMA_TX_CHANNEL_NUM,
				 (uintptr_t)data->tx_buffer, 0x0, buffer_len);
		if (err) {
			LOG_ERR("DMA reconfigure failed: %d", err);
			return err;
		}
	}

	return dma_start(config->dma, XILINX_AXI_DMA_TX_CHANNEL_NUM);
}

static void xilinx_axienet_isr(const struct device *dev)
{
	const struct xilinx_axienet_config *config = dev->config;
	struct xilinx_axienet_data *data = dev->data;
	uint32_t status =
		xilinx_axienet_read_register(config, XILINX_AXIENET_INTERRUPT_PENDING_OFFSET);

	(void)data;

	if (status & XILINX_AXIENET_INTERRUPT_PENDING_RXFIFOOVR_MASK) {
		LOG_WRN("FIFO was overrun - probably lost packets!");
		eth_stats_update_errors_rx(data->interface);
	} else if (status & XILINX_AXIENET_INTERRUPT_PENDING_RXRJECT_MASK) {
		/* this is extremely rare on Ethernet */
		/* most likely cause is mistake in FPGA configuration */
		LOG_WRN("Erroneous frame received!");
		eth_stats_update_errors_rx(data->interface);
	}

	if (status != 0) {
		/* clear IRQ by writing the same value back */
		xilinx_axienet_write_register(config, XILINX_AXIENET_INTERRUPT_STATUS_OFFSET,
					      status);
	}
}

static enum ethernet_hw_caps xilinx_axienet_caps(const struct device *dev)
{
	const struct xilinx_axienet_config *config = dev->config;
	enum ethernet_hw_caps ret = ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T |
				    ETHERNET_LINK_1000BASE_T | ETHERNET_HW_FILTERING;

	if (config->have_rx_csum_offload) {
		ret |= ETHERNET_HW_RX_CHKSUM_OFFLOAD;
	}
	if (config->have_tx_csum_offload) {
		ret |= ETHERNET_HW_TX_CHKSUM_OFFLOAD;
	}

	return ret;
}

static int xilinx_axienet_get_config(const struct device *dev, enum ethernet_config_type type,
				     struct ethernet_config *config)
{
	const struct xilinx_axienet_config *dev_config = dev->config;
	const struct xilinx_axienet_data *data = dev->data;
	struct phy_link_state link_state;
	int err;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_AUTO_NEG:
		/* enabled per default */
		config->auto_negotiation = true;
		return 0;
	case ETHERNET_CONFIG_TYPE_DUPLEX:
		err = phy_get_link_state(dev_config->phy, &link_state);
		if (err != 0) {
			LOG_ERR("Failed to get link state: %d", err);
			return err;
		}
		config->full_duplex = link_state.is_up && (link_state.speed & LINK_FULL_10BASE_T ||
							   link_state.speed & LINK_FULL_100BASE_T ||
							   link_state.speed & LINK_FULL_1000BASE_T);
		return 0;
	case ETHERNET_CONFIG_TYPE_LINK:
		err = phy_get_link_state(dev_config->phy, &link_state);
		if (err != 0) {
			LOG_ERR("Failed to get link state: %d", err);
			return err;
		}
		if (!link_state.is_up) {
			LOG_WRN("Ethernet is not up!");
			return -EAGAIN;
		}
		if (link_state.speed & LINK_HALF_10BASE_T ||
		    link_state.speed & LINK_FULL_10BASE_T) {
			config->l.link_10bt = true;
		}
		if (link_state.speed & LINK_HALF_100BASE_T ||
		    link_state.speed & LINK_FULL_100BASE_T) {
			config->l.link_100bt = true;
		}
		if (link_state.speed & LINK_HALF_1000BASE_T ||
		    link_state.speed & LINK_FULL_1000BASE_T) {
			config->l.link_1000bt = true;
		}
		return 0;
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(config->mac_address.addr, data->mac_addr, sizeof(data->mac_addr));
		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		/* not supported yet */
		config->promisc_mode = false;
		return 0;
	case ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT:
		if (dev_config->have_rx_csum_offload) {
			config->chksum_support =
				ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER |
				ETHERNET_CHECKSUM_SUPPORT_TCP | ETHERNET_CHECKSUM_SUPPORT_UDP |
				ETHERNET_CHECKSUM_SUPPORT_IPV6_HEADER |
				ETHERNET_CHECKSUM_SUPPORT_TCP | ETHERNET_CHECKSUM_SUPPORT_UDP;
		} else {
			config->chksum_support = ETHERNET_CHECKSUM_SUPPORT_NONE;
		}
		return 0;
	case ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT:
		if (dev_config->have_tx_csum_offload) {
			config->chksum_support =
				ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER |
				ETHERNET_CHECKSUM_SUPPORT_TCP | ETHERNET_CHECKSUM_SUPPORT_UDP |
				ETHERNET_CHECKSUM_SUPPORT_IPV6_HEADER |
				ETHERNET_CHECKSUM_SUPPORT_TCP | ETHERNET_CHECKSUM_SUPPORT_UDP;
		} else {
			config->chksum_support = ETHERNET_CHECKSUM_SUPPORT_NONE;
		}
		return 0;
	default:
		LOG_ERR("Unsupported configuration queried: %u", type);
		return -EINVAL;
	}
}

static void xilinx_axienet_iface_init(struct net_if *iface)
{
	struct xilinx_axienet_data *data = net_if_get_device(iface)->data;

	data->interface = iface;

	ethernet_init(iface);

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	LOG_INF("Interface initialized!");
}

static int xilinx_axienet_send(const struct device *dev, struct net_pkt *pkt)
{
	struct xilinx_axienet_data *data = dev->data;
	const struct xilinx_axienet_config *config = dev->config;
	size_t pkt_len = net_pkt_get_len(pkt);

	if (net_pkt_read(pkt, data->tx_buffer, pkt_len)) {
		LOG_ERR("Failed to read packet into TX buffer!");
		return -EIO;
	}
	return setup_dma_tx_transfer(dev, config, data, pkt_len);
}

static void xilinx_axienet_set_mac_address(const struct xilinx_axienet_config *config,
					   const struct xilinx_axienet_data *data)
{
	xilinx_axienet_write_register(config, XILINX_AXIENET_UNICAST_ADDRESS_WORD_0_OFFSET,
				      (data->mac_addr[0]) | (data->mac_addr[1] << 8) |
					      (data->mac_addr[2] << 16) |
					      (data->mac_addr[3] << 24));
	xilinx_axienet_write_register(config, XILINX_AXIENET_UNICAST_ADDRESS_WORD_1_OFFSET,
				      (data->mac_addr[4]) | (data->mac_addr[5] << 8));
}

static void phy_link_state_changed(const struct device *dev, struct phy_link_state *state,
				   void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	LOG_INF("Link state changed to: %s (speed %x)", state->is_up ? "up" : "down", state->speed);
}

static int xilinx_axienet_probe(const struct device *dev)
{
	const struct xilinx_axienet_config *config = dev->config;
	struct xilinx_axienet_data *data = dev->data;
	uint32_t status;
	struct phy_link_state link_state;
	int err;

	config->config_func(data);

	status = xilinx_axienet_read_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET);
	status = status & ~XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_RX_EN_MASK;
	xilinx_axienet_write_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET, status);

	/* RX disabled - it is safe to modify settings */

	/* clear any RX rejected interrupts from when the core was not configured */
	status = xilinx_axienet_read_register(config, XILINX_AXIENET_INTERRUPT_PENDING_OFFSET);
	if (status & XILINX_AXIENET_INTERRUPT_PENDING_RXRJECT_MASK) {
		xilinx_axienet_write_register(
			config, XILINX_AXIENET_INTERRUPT_STATUS_OFFSET,
			XILINX_AXIENET_INTERRUPT_STATUS_RXREJ_MASK |
				XILINX_AXIENET_INTERRUPT_STATUS_RXFIFOOVR_MASK);
	}

	xilinx_axienet_write_register(config, XILINX_AXIENET_INTERRUPT_ENABLE_OFFSET,
				      config->have_irq
					      ? XILINX_AXIENET_INTERRUPT_ENABLE_RXREJ_MASK |
							XILINX_AXIENET_INTERRUPT_ENABLE_OVR_MASK
					      : 0);

	xilinx_axienet_write_register(config,
				      XILINX_AXIENET_RECEIVER_CONFIGURATION_FLOW_CONTROL_OFFSET,
				      XILINX_AXIENET_RECEIVER_CONFIGURATION_FLOW_CONTROL_EN_MASK);

	err = phy_configure_link(config->phy, LINK_HALF_10BASE_T | LINK_FULL_10BASE_T |
						      LINK_HALF_100BASE_T | LINK_FULL_100BASE_T |
						      LINK_FULL_1000BASE_T);
	if (!config->phy || err) {
		LOG_ERR("Could not configure PHY: %d", config->phy ? err : -1);
	}

	err = phy_get_link_state(config->phy, &link_state);

	if (!config->phy || err) {
		LOG_ERR("Could not get PHY link state: %d", config->phy ? err : -1);
	} else {
		LOG_INF("Current link state: %s (speed %x)", link_state.is_up ? "up" : "down",
			link_state.speed);
	}

	err = phy_link_callback_set(config->phy, phy_link_state_changed, NULL);

	if (!config->phy || err) {
		LOG_ERR("Could not set PHY link state changed handler : %d",
			config->phy ? err : -1);
	}

	if (config->have_rx_csum_offload) {
		LOG_INF("RX Checksum offloading requested!");
	} else {
		LOG_INF("RX Checksum offloading disabled!");
	}

	if (config->have_tx_csum_offload) {
		LOG_INF("TX Checksum offloading requested!");
	} else {
		LOG_INF("TX Checksum offloading disabled!");
	}

	xilinx_axienet_set_mac_address(config, data);

	setup_dma_rx_transfer(dev, config, data);

	status = xilinx_axienet_read_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET);
	status = status | ~XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_RX_EN_MASK;
	xilinx_axienet_write_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET, status);

	status = xilinx_axienet_read_register(config, XILINX_AXIENET_TX_CONTROL_REG_OFFSET);
	status = status | XILINX_AXIENET_TX_CONTROL_TX_EN_MASK;
	xilinx_axienet_write_register(config, XILINX_AXIENET_TX_CONTROL_REG_OFFSET, status);

	return 0;
}

static const struct ethernet_api xilinx_axienet_api = {
	/* TODO PTP not supported yet */
	.iface_api.init = xilinx_axienet_iface_init,
	.get_capabilities = xilinx_axienet_caps,
	.get_config = xilinx_axienet_get_config,
	.send = xilinx_axienet_send,
};

#define SETUP_IRQS(inst)                                                                           \
	IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), xilinx_axienet_isr,           \
		    DEVICE_DT_INST_GET(inst), 0);                                                  \
                                                                                                   \
	irq_enable(DT_INST_IRQN(inst))

#define XILINX_AXIENET_INIT(inst)                                                                  \
                                                                                                   \
	static void xilinx_axienet_config_##inst(const struct xilinx_axienet_data *dev)            \
	{                                                                                          \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, interrupts), (SETUP_IRQS(inst)),           \
			    (LOG_INF("No IRQs defined!")));        \
	}                                                                                          \
                                                                                                   \
	static struct xilinx_axienet_data data_##inst = {                                          \
		.mac_addr = DT_INST_PROP(inst, local_mac_address),                                 \
		.dma_is_configured_rx = false,                                                     \
		.dma_is_configured_tx = false};                                                    \
	static const struct xilinx_axienet_config config_##inst = {                                \
		.config_func = xilinx_axienet_config_##inst,                                       \
		.dma = DEVICE_DT_GET(DT_INST_PHANDLE(inst, axistream_connected)),                  \
		.phy = DEVICE_DT_GET(DT_INST_PHANDLE(inst, phy_handle)),                           \
		.reg = (void *)(uintptr_t)DT_REG_ADDR(DT_INST_PARENT(inst)),                       \
		.have_irq = DT_INST_NODE_HAS_PROP(inst, interrupts),                               \
		.have_tx_csum_offload = DT_INST_PROP_OR(inst, xlnx_txcsum, 0x0) == 0x2,            \
		.have_rx_csum_offload = DT_INST_PROP_OR(inst, xlnx_rxcsum, 0x0) == 0x2,            \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, xilinx_axienet_probe, NULL, &data_##inst,              \
				      &config_##inst, CONFIG_ETH_INIT_PRIORITY,                    \
				      &xilinx_axienet_api, NET_ETH_MTU);

/* two different compatibles match the very same Ethernet core */

#define DT_DRV_COMPAT xlnx_axi_ethernet_7_2
DT_INST_FOREACH_STATUS_OKAY(XILINX_AXIENET_INIT);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT xlnx_axi_ethernet_1_00_a
DT_INST_FOREACH_STATUS_OKAY(XILINX_AXIENET_INIT);
