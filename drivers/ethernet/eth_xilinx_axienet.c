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

#define XILINX_AXIENET_INTERRUPT_ENABLE_OFFSET     0x00000014
#define XILINX_AXIENET_INTERRUPT_ENABLE_RXREJ_MASK 0x00000008
#define XILINX_AXIENET_INTERRUPT_ENABLE_OVR_MASK   0x00000010 /* FIFO overrun */

#define XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_0_REG_OFFSET     0x00000400
#define XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET     0x00000404
#define XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_RX_EN_MASK 0x10000000
#define XILINX_AXIENET_RECEIVER_CONFIGURATION_FLOW_CONTROL_OFFSET   0x0000040C
#define XILINX_AXIENET_RECEIVER_CONFIGURATION_FLOW_CONTROL_EN_MASK  0x20000000
#define XILINX_AXIENET_TX_CONTROL_REG_OFFSET                        0x00000408
#define XILINX_AXIENET_TX_CONTROL_TX_EN_MASK                        (1 << 11)

#define XILINX_AXIENET_UNICAST_ADDRESS_WORD_0_OFFSET 0x00000700
#define XILINX_AXIENET_UNICAST_ADDRESS_WORD_1_OFFSET 0x00000704

#if (CONFIG_DCACHE_LINE_SIZE > 0)
/* cache-line aligned to allow selective cache-line invalidation on the buffer */
#define XILINX_AXIENET_ETH_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
/* pointer-aligned to reduce padding in the struct */
#define XILINX_AXIENET_ETH_ALIGN sizeof(void *)
#endif

#define XILINX_AXIENET_ETH_BUFFER_SIZE                                                             \
	((NET_ETH_MAX_FRAME_SIZE + XILINX_AXIENET_ETH_ALIGN - 1) & ~(XILINX_AXIENET_ETH_ALIGN - 1))

struct xilinx_axienet_buffer {
	uint8_t buffer[XILINX_AXIENET_ETH_BUFFER_SIZE];
} __aligned(XILINX_AXIENET_ETH_ALIGN);

/* device state */
struct xilinx_axienet_data {
	struct xilinx_axienet_buffer tx_buffer[CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_TX];
	struct xilinx_axienet_buffer rx_buffer[CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX];

	size_t rx_populated_buffer_index;
	size_t rx_completed_buffer_index;
	size_t tx_populated_buffer_index;
	size_t tx_completed_buffer_index;

	struct net_if *interface;

	/* device mac address */
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	bool dma_is_configured_rx;
	bool dma_is_configured_tx;
};

/* global configuration per Ethernet device */
struct xilinx_axienet_config {
	void (*config_func)(const struct xilinx_axienet_data *dev);
	const struct device *dma;

	const struct device *phy;

	mem_addr_t reg;

	int irq_num;
	bool have_irq;

	bool have_rx_csum_offload;
	bool have_tx_csum_offload;
};

static void xilinx_axienet_write_register(const struct xilinx_axienet_config *config,
					  mem_addr_t reg_offset, uint32_t value)
{
	sys_write32(value, config->reg + reg_offset);
}

static uint32_t xilinx_axienet_read_register(const struct xilinx_axienet_config *config,
					     mem_addr_t reg_offset)
{
	return sys_read32(config->reg + reg_offset);
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

	size_t next_descriptor =
		(data->rx_completed_buffer_index + 1) % CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX;
	size_t current_descriptor = data->rx_completed_buffer_index;

	if (!net_if_is_up(data->interface)) {
		/*
		 * cannot receive data now, so discard silently
		 * setup new transfer for when the interface is back up
		 */
		goto setup_new_transfer;
	}

	if (status < 0) {
		LOG_ERR("DMA RX error: %d", status);
		eth_stats_update_errors_rx(data->interface);
		goto setup_new_transfer;
	}

	data->rx_completed_buffer_index = next_descriptor;

	packet_size = dma_xilinx_axi_dma_last_received_frame_length(dma);
	pkt = net_pkt_rx_alloc_with_buffer(data->interface, packet_size, AF_UNSPEC, 0, K_NO_WAIT);

	if (!pkt) {
		LOG_ERR("Could not allocate a packet!");
		goto setup_new_transfer;
	}
	if (net_pkt_write(pkt, data->rx_buffer[current_descriptor].buffer, packet_size)) {
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
	size_t next_descriptor =
		(data->tx_completed_buffer_index + 1) % CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_TX;

	data->tx_completed_buffer_index = next_descriptor;

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
	size_t next_descriptor =
		(data->rx_populated_buffer_index + 1) % CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX;
	size_t current_descriptor = data->rx_populated_buffer_index;

	if (next_descriptor == data->rx_completed_buffer_index) {
		LOG_ERR("Cannot start RX via DMA - populated buffer %zu will run into completed"
			" buffer %zu!",
			data->rx_populated_buffer_index, data->rx_completed_buffer_index);
		return -ENOSPC;
	}

	if (!data->dma_is_configured_rx) {
		struct dma_block_config head_block = {
			.source_address = 0x0,
			.dest_address = (uintptr_t)data->rx_buffer[current_descriptor].buffer,
			.block_size = sizeof(data->rx_buffer[current_descriptor].buffer),
			.next_block = NULL,
			.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
			.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT};
		struct dma_config dma_conf = {.dma_slot = 0,
					      .channel_direction = PERIPHERAL_TO_MEMORY,
					      .complete_callback_en = 1,
					      .error_callback_dis = 0,
					      .block_count = 1,
					      .head_block = &head_block,
					      .user_data = (void *)dev,
					      .dma_callback = xilinx_axienet_rx_callback};

		if (config->have_rx_csum_offload) {
			dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_FULL_CSUM_OFFLOAD;
		} else {
			dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_NO_CSUM_OFFLOAD;
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
				 (uintptr_t)data->rx_buffer[current_descriptor].buffer,
				 sizeof(data->rx_buffer[current_descriptor].buffer));
		if (err) {
			LOG_ERR("DMA reconfigure failed: %d", err);
			return err;
		}
	}
	LOG_DBG("Receiving one packet with DMA!");

	/* prevent concurrent modification */
	data->rx_populated_buffer_index = next_descriptor;

	err = dma_start(config->dma, XILINX_AXI_DMA_RX_CHANNEL_NUM);

	if (err) {
		/* buffer has not been accepted by DMA */
		data->rx_populated_buffer_index = current_descriptor;
	}

	return err;
}

/* assumes that the caller has set up data->tx_buffer */
static int setup_dma_tx_transfer(const struct device *dev,
				 const struct xilinx_axienet_config *config,
				 struct xilinx_axienet_data *data, uint32_t buffer_len)
{
	int err;
	size_t next_descriptor =
		(data->tx_populated_buffer_index + 1) % CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_TX;
	size_t current_descriptor = data->tx_populated_buffer_index;

	if (next_descriptor == data->tx_completed_buffer_index) {
		LOG_ERR("Cannot start TX via DMA - populated buffer %zu will run into completed"
			" buffer %zu!",
			data->tx_populated_buffer_index, data->tx_completed_buffer_index);
		return -ENOSPC;
	}

	if (!data->dma_is_configured_tx) {
		struct dma_block_config head_block = {
			.source_address = (uintptr_t)data->tx_buffer[current_descriptor].buffer,
			.dest_address = 0x0,
			.block_size = buffer_len,
			.next_block = NULL,
			.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
			.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT};
		struct dma_config dma_conf = {.dma_slot = 0,
					      .channel_direction = MEMORY_TO_PERIPHERAL,
					      .complete_callback_en = 1,
					      .error_callback_dis = 0,
					      .block_count = 1,
					      .head_block = &head_block,
					      .user_data = (void *)dev,
					      .dma_callback = xilinx_axienet_tx_callback};

		if (config->have_tx_csum_offload) {
			dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_FULL_CSUM_OFFLOAD;
		} else {
			dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_NO_CSUM_OFFLOAD;
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
				 (uintptr_t)data->tx_buffer[current_descriptor].buffer, 0x0,
				 buffer_len);
		if (err) {
			LOG_ERR("DMA reconfigure failed: %d", err);
			return err;
		}
	}

	/* prevent concurrent modification */
	data->tx_populated_buffer_index = next_descriptor;

	err = dma_start(config->dma, XILINX_AXI_DMA_TX_CHANNEL_NUM);

	if (err) {
		/* buffer has not been accepted by DMA */
		data->tx_populated_buffer_index = current_descriptor;
	}

	return err;
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
				    ETHERNET_LINK_1000BASE_T;

	if (config->have_rx_csum_offload) {
		ret |= ETHERNET_HW_RX_CHKSUM_OFFLOAD;
	}
	if (config->have_tx_csum_offload) {
		ret |= ETHERNET_HW_TX_CHKSUM_OFFLOAD;
	}

	return ret;
}

static const struct device *xilinx_axienet_get_phy(const struct device *dev)
{
	const struct xilinx_axienet_config *config = dev->config;

	return config->phy;
}

static int xilinx_axienet_get_config(const struct device *dev, enum ethernet_config_type type,
				     struct ethernet_config *config)
{
	const struct xilinx_axienet_config *dev_config = dev->config;
	const struct xilinx_axienet_data *data = dev->data;
	struct phy_link_state link_state;
	int err;

	switch (type) {
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

static int xilinx_axienet_set_config(const struct device *dev, enum ethernet_config_type type,
				     const struct ethernet_config *config)
{
	const struct xilinx_axienet_config *dev_config = dev->config;
	struct xilinx_axienet_data *data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		xilinx_axienet_set_mac_address(dev_config, data);
		return net_if_set_link_addr(data->interface, data->mac_addr,
			sizeof(data->mac_addr), NET_LINK_ETHERNET);
	default:
		LOG_ERR("Unsupported configuration set: %u", type);
		return -EINVAL;
	}
}

static void phy_link_state_changed(const struct device *dev, struct phy_link_state *state,
				   void *user_data)
{
	struct xilinx_axienet_data *data = user_data;

	ARG_UNUSED(dev);

	LOG_INF("Link state changed to: %s (speed %x)", state->is_up ? "up" : "down", state->speed);

	/* inform the L2 driver about link event */
	if (state->is_up) {
		net_eth_carrier_on(data->interface);
	} else {
		net_eth_carrier_off(data->interface);
	}
}

static void xilinx_axienet_iface_init(struct net_if *iface)
{
	struct xilinx_axienet_data *data = net_if_get_device(iface)->data;
	const struct xilinx_axienet_config *config = net_if_get_device(iface)->config;
	int err;

	data->interface = iface;

	ethernet_init(iface);

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	/* carrier is initially off */
	net_eth_carrier_off(iface);

	err = phy_link_callback_set(config->phy, phy_link_state_changed, data);

	if (err) {
		LOG_ERR("Could not set PHY link state changed handler : %d",
			config->phy ? err : -1);
	}

	LOG_INF("Interface initialized!");
}

static int xilinx_axienet_send(const struct device *dev, struct net_pkt *pkt)
{
	struct xilinx_axienet_data *data = dev->data;
	const struct xilinx_axienet_config *config = dev->config;
	size_t pkt_len = net_pkt_get_len(pkt);
	size_t current_descriptor = data->tx_populated_buffer_index;

	if (net_pkt_read(pkt, data->tx_buffer[current_descriptor].buffer, pkt_len)) {
		LOG_ERR("Failed to read packet into TX buffer!");
		return -EIO;
	}
	return setup_dma_tx_transfer(dev, config, data, pkt_len);
}

static int xilinx_axienet_probe(const struct device *dev)
{
	const struct xilinx_axienet_config *config = dev->config;
	struct xilinx_axienet_data *data = dev->data;
	uint32_t status;
	int err;

	status = xilinx_axienet_read_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET);
	status = status & ~XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_RX_EN_MASK;
	xilinx_axienet_write_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET, status);

	/* RX disabled - it is safe to modify settings */

	/* clear any RX rejected interrupts from when the core was not configured */
	xilinx_axienet_write_register(config, XILINX_AXIENET_INTERRUPT_STATUS_OFFSET,
				      XILINX_AXIENET_INTERRUPT_STATUS_RXREJ_MASK |
					      XILINX_AXIENET_INTERRUPT_STATUS_RXFIFOOVR_MASK);

	xilinx_axienet_write_register(config, XILINX_AXIENET_INTERRUPT_ENABLE_OFFSET,
				      config->have_irq
					      ? XILINX_AXIENET_INTERRUPT_ENABLE_RXREJ_MASK |
							XILINX_AXIENET_INTERRUPT_ENABLE_OVR_MASK
					      : 0);

	xilinx_axienet_write_register(config,
				      XILINX_AXIENET_RECEIVER_CONFIGURATION_FLOW_CONTROL_OFFSET,
				      XILINX_AXIENET_RECEIVER_CONFIGURATION_FLOW_CONTROL_EN_MASK);

	/* at time of writing, hardware does not support half duplex */
	err = phy_configure_link(config->phy, LINK_FULL_10BASE_T | LINK_FULL_100BASE_T |
						      LINK_FULL_1000BASE_T);
	if (err) {
		LOG_WRN("Could not configure PHY: %d", -err);
	}

	LOG_INF("RX Checksum offloading %s",
		config->have_rx_csum_offload ? "requested" : "disabled");
	LOG_INF("TX Checksum offloading %s",
		config->have_tx_csum_offload ? "requested" : "disabled");

	xilinx_axienet_set_mac_address(config, data);

	for (int i = 0; i < CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX - 1; i++) {
		setup_dma_rx_transfer(dev, config, data);
	}

	status = xilinx_axienet_read_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET);
	status = status | XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_RX_EN_MASK;
	xilinx_axienet_write_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET, status);

	status = xilinx_axienet_read_register(config, XILINX_AXIENET_TX_CONTROL_REG_OFFSET);
	status = status | XILINX_AXIENET_TX_CONTROL_TX_EN_MASK;
	xilinx_axienet_write_register(config, XILINX_AXIENET_TX_CONTROL_REG_OFFSET, status);

	config->config_func(data);

	return 0;
}

/* TODO PTP, VLAN not supported yet */
static const struct ethernet_api xilinx_axienet_api = {
	.iface_api.init = xilinx_axienet_iface_init,
	.get_capabilities = xilinx_axienet_caps,
	.get_config = xilinx_axienet_get_config,
	.set_config = xilinx_axienet_set_config,
	.get_phy = xilinx_axienet_get_phy,
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
		.reg = DT_REG_ADDR(DT_INST_PARENT(inst)),                                          \
		.have_irq = DT_INST_NODE_HAS_PROP(inst, interrupts),                               \
		.have_tx_csum_offload = DT_INST_PROP_OR(inst, xlnx_txcsum, 0x0) == 0x2,            \
		.have_rx_csum_offload = DT_INST_PROP_OR(inst, xlnx_rxcsum, 0x0) == 0x2,            \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, xilinx_axienet_probe, NULL, &data_##inst,              \
				      &config_##inst, CONFIG_ETH_INIT_PRIORITY,                    \
				      &xilinx_axienet_api, NET_ETH_MTU);

#define DT_DRV_COMPAT xlnx_axi_ethernet_1_00_a
DT_INST_FOREACH_STATUS_OKAY(XILINX_AXIENET_INIT);
