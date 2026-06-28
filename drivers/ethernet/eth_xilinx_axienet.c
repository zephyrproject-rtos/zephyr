/*
 * Xilinx AXI 1G / 2.5G Ethernet Subsystem
 *
 * Copyright (c) 2024, CISPA Helmholtz Center for Information Security
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
#include "eth.h"

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
#define XILINX_AXIENET_TX_CONTROL_TX_EN_MASK                        BIT(28)

#define XILINX_AXIENET_UNICAST_ADDRESS_WORD_0_OFFSET 0x00000700
#define XILINX_AXIENET_UNICAST_ADDRESS_WORD_1_OFFSET 0x00000704

/* Xilinx OUI (Organizationally Unique Identifier) for MAC */
#define XILINX_OUI_BYTE_0  0x00
#define XILINX_OUI_BYTE_1  0x0A
#define XILINX_OUI_BYTE_2  0x35

#if (CONFIG_DCACHE_LINE_SIZE > 0)
/* cache-line aligned to allow selective cache-line invalidation on the buffer */
#define XILINX_AXIENET_ETH_ALIGN CONFIG_DCACHE_LINE_SIZE
#else
/* pointer-aligned to reduce padding in the struct */
#define XILINX_AXIENET_ETH_ALIGN sizeof(void *)
#endif

#define XILINX_AXIENET_ETH_BUFFER_SIZE                                                             \
	((NET_ETH_MAX_FRAME_SIZE + XILINX_AXIENET_ETH_ALIGN - 1) & ~(XILINX_AXIENET_ETH_ALIGN - 1))

/*
 * AxiEnet RX does not split frames across net_buf fragments; each RX
 * descriptor uses a single buffer sized for NET_ETH_MAX_FRAME_SIZE.
 */
BUILD_ASSERT(CONFIG_NET_BUF_DATA_SIZE >= XILINX_AXIENET_ETH_BUFFER_SIZE,
	     "AxiEnet RX needs larger CONFIG_NET_BUF_DATA_SIZE (full frame per buf)");

/*
 * Maximum DMA SG blocks per TX packet (one per net_buf in net_pkt->frags).
 *
 * When CONFIG_NET_L2_ETHERNET_RESERVE_HEADER is disabled (default), the L2
 * layer allocates the Ethernet header in a separate small net_buf; payload
 * fragments follow, so add 1 to the payload fragment count.
 * When enabled, the header is reserved in the first data net_buf via
 * net_buf_push().
 */
#if IS_ENABLED(CONFIG_NET_L2_ETHERNET_RESERVE_HEADER)
#define XILINX_AXIENET_TX_MAX_FRAGS \
	DIV_ROUND_UP(NET_ETH_MAX_FRAME_SIZE, CONFIG_NET_BUF_DATA_SIZE)
#else
#define XILINX_AXIENET_TX_MAX_FRAGS \
	(1 + DIV_ROUND_UP(NET_ETH_MAX_FRAME_SIZE - NET_ETH_MAX_HDR_SIZE, \
			  CONFIG_NET_BUF_DATA_SIZE))
#endif

/* device state */
struct xilinx_axienet_data {
	/* TX: net_pkt allocated by stack, DMA reads it directly without memcpy */
	struct net_pkt *tx_pkt[CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_TX];

	/* RX buf: reserved from networking stack via net_pkt_get_reserve_rx_data() */
	struct net_buf *rx_buf[CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX];

	size_t rx_populated_buffer_index;
	size_t rx_completed_buffer_index;
	size_t tx_populated_buffer_index;
	size_t tx_completed_buffer_index;

	struct net_if *interface;

	/* device mac address */
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	bool dma_is_configured_rx;
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

	bool have_random_mac;
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
	size_t current_rx_desc_idx = data->rx_completed_buffer_index;
	struct net_buf *recvd_buf;
	struct net_pkt *stack_rx_pkt;

	data->rx_completed_buffer_index =
		(current_rx_desc_idx + 1) % CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX;

	/* Take the reserved RX buffer containing the frame written by DMA. */
	recvd_buf = data->rx_buf[current_rx_desc_idx];
	/* Clear slot; next DMA RX transfer assigns a new reserved buffer here. */
	data->rx_buf[current_rx_desc_idx] = NULL;

	if (!net_if_is_up(data->interface)) {
		net_pkt_frag_unref(recvd_buf);
		if (setup_dma_rx_transfer(ethdev, ethdev->config, data) != 0) {
			LOG_ERR("Could not set up next RX DMA transfer!");
		}
		return;
	}

	if (status < 0) {
		LOG_ERR("DMA RX error: %d", status);
		eth_stats_update_errors_rx(data->interface);
		net_pkt_frag_unref(recvd_buf);
		if (setup_dma_rx_transfer(ethdev, ethdev->config, data) != 0) {
			LOG_ERR("Could not set up next RX DMA transfer!");
		}
		return;
	}

	/*
	 * Prepare the next RX descriptor before handing recvd_buf to the stack.
	 * Requires adequate CONFIG_NET_BUF_RX_COUNT.
	 */
	int setup_err = setup_dma_rx_transfer(ethdev, ethdev->config, data);

	if (setup_err != 0) {
		if (setup_err == -ENOBUFS) {
			eth_stats_update_errors_rx(data->interface);
		} else {
			LOG_ERR("Next RX DMA prepare failed: %d", setup_err);
		}
		net_pkt_frag_unref(recvd_buf);
		return;
	}

	recvd_buf->len = dma_xilinx_axi_dma_last_received_frame_length(dma);

	/* Allocate net_pkt for received frame; no internal data buffer allocated. */
	stack_rx_pkt = net_pkt_rx_alloc_on_iface(data->interface, K_NO_WAIT);
	if (!stack_rx_pkt) {
		LOG_ERR("Could not allocate a packet!");
		net_pkt_frag_unref(recvd_buf);
		return;
	}

	/* Link recvd_buf into pkt fragment chain */
	net_pkt_frag_add(stack_rx_pkt, recvd_buf);

	/* Pass pkt up to the network stack/ip/l3 processing. */
	if (net_recv_data(data->interface, stack_rx_pkt) < 0) {
		LOG_ERR("Could not receive packet data!");
		net_pkt_unref(stack_rx_pkt);
		return;
	}
}

static void xilinx_axienet_tx_callback(const struct device *dev, void *user_data, uint32_t channel,
				       int status)
{
	struct device *ethdev = (struct device *)user_data;
	struct xilinx_axienet_data *data = ethdev->data;
	size_t current_tx_desc_idx = data->tx_completed_buffer_index;

	data->tx_completed_buffer_index =
		(current_tx_desc_idx + 1) % CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_TX;

	/* Release the net_pkt now that DMA has finished reading all its fragments. */
	if (data->tx_pkt[current_tx_desc_idx]) {
		net_pkt_unref(data->tx_pkt[current_tx_desc_idx]);
		data->tx_pkt[current_tx_desc_idx] = NULL;
	}

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
	size_t current_rx_desc_idx = data->rx_populated_buffer_index;
	size_t next_rx_desc_idx =
		(current_rx_desc_idx + 1) % CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX;

	if (next_rx_desc_idx == data->rx_completed_buffer_index) {
		LOG_ERR("Cannot start RX via DMA - populated buffer %zu will run into completed"
			" buffer %zu!",
			data->rx_populated_buffer_index, data->rx_completed_buffer_index);
		return -ENOSPC;
	}

	/* Reserve an RX buffer from the networking stack pool for this DMA slot. */
	struct net_buf *rx_new_reserved_buf =
		net_pkt_get_reserve_rx_data(CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);

	if (!rx_new_reserved_buf) {
		LOG_WRN("Networking stack RX buffers exhausted");
		return -ENOBUFS;
	}

	/* Save for xilinx_axienet_rx_callback; DMA writes frame into this net_buf. */
	data->rx_buf[current_rx_desc_idx] = rx_new_reserved_buf;

	if (!data->dma_is_configured_rx) {
		struct dma_block_config head_block = {
			.source_address = 0x0,
			.dest_address = (uintptr_t)rx_new_reserved_buf->data,
			.block_size = rx_new_reserved_buf->size,
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
			net_pkt_frag_unref(rx_new_reserved_buf);
			data->rx_buf[current_rx_desc_idx] = NULL;
			return err;
		}

		data->dma_is_configured_rx = true;
	} else {
		/* Fast reload: only the destination buffer address changes. */
		err = dma_reload(config->dma, XILINX_AXI_DMA_RX_CHANNEL_NUM, 0x0,
				 (uintptr_t)rx_new_reserved_buf->data, rx_new_reserved_buf->size);
		if (err) {
			LOG_ERR("DMA reload failed: %d", err);
			net_pkt_frag_unref(rx_new_reserved_buf);
			data->rx_buf[current_rx_desc_idx] = NULL;
			return err;
		}
	}

	data->rx_populated_buffer_index = next_rx_desc_idx;

	err = dma_start(config->dma, XILINX_AXI_DMA_RX_CHANNEL_NUM);
	if (err) {
		/* DMA did not accept the buffer — undo. */
		data->rx_populated_buffer_index = current_rx_desc_idx;
		net_pkt_frag_unref(rx_new_reserved_buf);
		data->rx_buf[current_rx_desc_idx] = NULL;
		return err;
	}

	return 0;
}

static int setup_dma_tx_transfer(const struct device *dev,
				 const struct xilinx_axienet_config *config,
				 struct xilinx_axienet_data *data,
				 struct net_pkt *outgoing_pkt)
{
	int err;
	size_t current_tx_desc_idx = data->tx_populated_buffer_index;
	size_t next_tx_desc_idx =
		(current_tx_desc_idx + 1) % CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_TX;

	if (next_tx_desc_idx == data->tx_completed_buffer_index) {
		LOG_ERR("Cannot start TX via DMA - populated buffer %zu will run into completed"
			" buffer %zu!",
			data->tx_populated_buffer_index, data->tx_completed_buffer_index);
		return -ENOSPC;
	}

	/* Build a dma_block_config chain from net_pkt fragments */
	struct dma_block_config tx_blocks[XILINX_AXIENET_TX_MAX_FRAGS];
	uint8_t nfrags = 0;
	struct net_buf *tx_pkt_frag = outgoing_pkt->frags;

	/* Each net_buf fragment maps to one DMA block; chain linked via next_block. */
	while (tx_pkt_frag && nfrags < XILINX_AXIENET_TX_MAX_FRAGS) {
		tx_blocks[nfrags].source_address = (uintptr_t)tx_pkt_frag->data;
		tx_blocks[nfrags].dest_address   = 0x0;
		tx_blocks[nfrags].block_size     = tx_pkt_frag->len;
		tx_blocks[nfrags].next_block     =
			(tx_pkt_frag->frags && (nfrags + 1 < XILINX_AXIENET_TX_MAX_FRAGS))
			? &tx_blocks[nfrags + 1] : NULL;
		tx_blocks[nfrags].source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		tx_blocks[nfrags].dest_addr_adj   = DMA_ADDR_ADJ_INCREMENT;
		/* One more fragment added to tx_blocks array. */
		nfrags++;
		/* Move to the next net_buf fragment in the tx pkt chain. */
		tx_pkt_frag = tx_pkt_frag->frags;
	}

	/* tx_pkt_frag non-NULL: loop hit MAX_FRAGS before reaching end of chain. */
	if (tx_pkt_frag) {
		LOG_ERR("TX packet has too many fragments (max %d)", XILINX_AXIENET_TX_MAX_FRAGS);
		return -EMSGSIZE;
	}

	struct dma_config dma_conf = {.dma_slot = 0,
				      .channel_direction = MEMORY_TO_PERIPHERAL,
				      .complete_callback_en = 1,
				      .error_callback_dis = 0,
				      .block_count = nfrags,
				      .head_block = &tx_blocks[0],
				      .user_data = (void *)dev,
				      .dma_callback = xilinx_axienet_tx_callback};

	if (config->have_tx_csum_offload) {
		dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_FULL_CSUM_OFFLOAD;
	} else {
		dma_conf.linked_channel = XILINX_AXI_DMA_LINKED_CHANNEL_NO_CSUM_OFFLOAD;
	}

	err = dma_config(config->dma, XILINX_AXI_DMA_TX_CHANNEL_NUM, &dma_conf);
	if (err) {
		LOG_ERR("DMA TX config failed: %d", err);
		return err;
	}

	data->tx_populated_buffer_index = next_tx_desc_idx;

	err = dma_start(config->dma, XILINX_AXI_DMA_TX_CHANNEL_NUM);
	if (err) {
		LOG_ERR("DMA TX start failed: %d", err);
		data->tx_populated_buffer_index = current_tx_desc_idx;
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

static enum ethernet_hw_caps xilinx_axienet_caps(const struct device *dev,
						 struct net_if *iface __unused)
{
	const struct xilinx_axienet_config *config = dev->config;
	enum ethernet_hw_caps ret = ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE |
				    ETHERNET_LINK_1000BASE;

	if (config->have_rx_csum_offload) {
		ret |= ETHERNET_HW_RX_CHKSUM_OFFLOAD;
	}
	if (config->have_tx_csum_offload) {
		ret |= ETHERNET_HW_TX_CHKSUM_OFFLOAD;
	}

	return ret;
}

static const struct device *xilinx_axienet_get_phy(const struct device *dev,
						   struct net_if *iface __unused)
{
	const struct xilinx_axienet_config *config = dev->config;

	return config->phy;
}

static int xilinx_axienet_get_config(const struct device *dev,
				     struct net_if *iface __unused,
				     enum ethernet_config_type type,
				     struct ethernet_config *config)
{
	const struct xilinx_axienet_config *dev_config = dev->config;

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

static int xilinx_axienet_set_config(const struct device *dev,
				     struct net_if *iface __unused,
				     enum ethernet_config_type type,
				     const struct ethernet_config *config)
{
	const struct xilinx_axienet_config *dev_config = dev->config;
	struct xilinx_axienet_data *data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		xilinx_axienet_set_mac_address(dev_config, data);
		return 0;
	default:
		LOG_ERR("Unsupported configuration set: %u", type);
		return -EINVAL;
	}
}

static void phy_link_state_changed(const struct device *dev __unused, struct phy_link_state *state,
				   void *user_data)
{
	struct net_if *iface = (struct net_if *)user_data;

	/* inform the L2 driver about link event */
	net_eth_carrier_set(iface, state->is_up);
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

	err = phy_link_callback_set(config->phy, phy_link_state_changed, iface);

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
	size_t current_tx_desc_idx = data->tx_populated_buffer_index;
	int err;

	/* Increment pkt refcount; DMA reads frags directly, unref'd in tx_callback. */
	net_pkt_ref(pkt);
	/* Save pkt pointer for this desc_idx; cleared in tx_callback. */
	data->tx_pkt[current_tx_desc_idx] = pkt;

	err = setup_dma_tx_transfer(dev, config, data, pkt);
	if (err) {
		data->tx_pkt[current_tx_desc_idx] = NULL;
		net_pkt_unref(pkt);
	}

	return err;
}

static int xilinx_axienet_probe(const struct device *dev)
{
	const struct xilinx_axienet_config *config = dev->config;
	struct xilinx_axienet_data *data = dev->data;
	uint32_t status;

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

	LOG_INF("RX Checksum offloading %s",
		config->have_rx_csum_offload ? "requested" : "disabled");
	LOG_INF("TX Checksum offloading %s",
		config->have_tx_csum_offload ? "requested" : "disabled");

	if (config->have_random_mac) {
		gen_random_mac(data->mac_addr,
			      XILINX_OUI_BYTE_0, XILINX_OUI_BYTE_1, XILINX_OUI_BYTE_2);
	}

	xilinx_axienet_set_mac_address(config, data);

	config->config_func(data);

	return 0;
}

static int xilinx_axienet_stop(const struct device *dev, struct net_if *iface __unused)
{
	const struct xilinx_axienet_config *config = dev->config;
	struct xilinx_axienet_data *data = dev->data;
	uint32_t status;
	int err;

	/* Disable AXIENET RX */
	status = xilinx_axienet_read_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET);
	status &= ~XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_RX_EN_MASK;
	xilinx_axienet_write_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET, status);

	/* Stop the RX DMA channel */
	err = dma_stop(config->dma, XILINX_AXI_DMA_RX_CHANNEL_NUM);
	if (err) {
		LOG_ERR("%s: failed to stop RX DMA: %d", dev->name, err);
	}

	/* Stop the TX DMA channel */
	err = dma_stop(config->dma, XILINX_AXI_DMA_TX_CHANNEL_NUM);
	if (err) {
		LOG_ERR("%s: failed to stop TX DMA: %d", dev->name, err);
	}

	/* Disable AXIENET TX */
	status = xilinx_axienet_read_register(config, XILINX_AXIENET_TX_CONTROL_REG_OFFSET);
	status &= ~XILINX_AXIENET_TX_CONTROL_TX_EN_MASK;
	xilinx_axienet_write_register(config, XILINX_AXIENET_TX_CONTROL_REG_OFFSET, status);

	/* Release any in-flight TX net_pkts (DMA has stopped; callbacks won't fire). */
	for (int i = 0; i < CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_TX; i++) {
		if (data->tx_pkt[i]) {
			net_pkt_unref(data->tx_pkt[i]);
			data->tx_pkt[i] = NULL;
		}
	}

	/* Return any reserved RX net_bufs still held in the DMA ring. */
	for (int i = 0; i < CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX; i++) {
		if (data->rx_buf[i]) {
			net_pkt_frag_unref(data->rx_buf[i]);
			data->rx_buf[i] = NULL;
		}
	}

	data->dma_is_configured_rx = false;
	data->rx_populated_buffer_index = 0;
	data->rx_completed_buffer_index = 0;
	data->tx_populated_buffer_index = 0;
	data->tx_completed_buffer_index = 0;

	LOG_INF("%s: AxiEnet stopped", dev->name);
	return 0;
}

static int xilinx_axienet_start(const struct device *dev, struct net_if *iface __unused)
{
	const struct xilinx_axienet_config *config = dev->config;
	struct xilinx_axienet_data *data = dev->data;
	uint32_t status;
	int err;

	/* Initialize the RX DMA ring; each call reserves one networking stack RX buffer. */
	for (int i = 0; i < CONFIG_ETH_XILINX_AXIENET_BUFFER_NUM_RX - 1; i++) {
		err = setup_dma_rx_transfer(dev, config, data);
		if (err) {
			LOG_ERR("%s: failed to initialize RX DMA transfer %d: %d", dev->name, i,
				err);
			data->dma_is_configured_rx = false;
			dma_stop(config->dma, XILINX_AXI_DMA_RX_CHANNEL_NUM);
			return err;
		}
	}

	/* Enable AXIENET RX */
	status = xilinx_axienet_read_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET);
	status = status | XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_RX_EN_MASK;
	xilinx_axienet_write_register(
		config, XILINX_AXIENET_RECEIVER_CONFIGURATION_WORD_1_REG_OFFSET, status);

	/* Enable AXIENET TX */
	status = xilinx_axienet_read_register(config, XILINX_AXIENET_TX_CONTROL_REG_OFFSET);
	status = status | XILINX_AXIENET_TX_CONTROL_TX_EN_MASK;
	xilinx_axienet_write_register(config, XILINX_AXIENET_TX_CONTROL_REG_OFFSET, status);

	LOG_INF("%s: AxiEnet started", dev->name);
	return 0;
}

/* TODO PTP, VLAN not supported yet */
static const struct ethernet_api xilinx_axienet_api = {
	.iface_api.init = xilinx_axienet_iface_init,
	.start            = xilinx_axienet_start,
	.stop             = xilinx_axienet_stop,
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
		.mac_addr = DT_INST_PROP_OR(inst, local_mac_address, {0}),                         \
		.dma_is_configured_rx = false,                                                     \
	};                                                                                         \
	static const struct xilinx_axienet_config config_##inst = {                                \
		.config_func = xilinx_axienet_config_##inst,                                       \
		.dma = DEVICE_DT_GET(DT_INST_PHANDLE(inst, axistream_connected)),                  \
		.phy = DEVICE_DT_GET(DT_INST_PHANDLE(inst, phy_handle)),                           \
		.reg = DT_REG_ADDR(DT_INST_PARENT(inst)),                                          \
		.have_irq = DT_INST_NODE_HAS_PROP(inst, interrupts),                               \
		.have_tx_csum_offload = DT_INST_PROP_OR(inst, xlnx_txcsum, 0x0) == 0x2,            \
		.have_rx_csum_offload = DT_INST_PROP_OR(inst, xlnx_rxcsum, 0x0) == 0x2,            \
		.have_random_mac = DT_INST_PROP(inst, zephyr_random_mac_address),                  \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, xilinx_axienet_probe, NULL, &data_##inst,              \
				      &config_##inst, CONFIG_ETH_INIT_PRIORITY,                    \
				      &xilinx_axienet_api, NET_ETH_MTU);

#define DT_DRV_COMPAT xlnx_axi_ethernet_1_00_a
DT_INST_FOREACH_STATUS_OKAY(XILINX_AXIENET_INIT);
