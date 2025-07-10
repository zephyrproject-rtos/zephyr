/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_intel_igc, CONFIG_ETHERNET_LOG_LEVEL);

#include <errno.h>
#include <soc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/crc.h>
#include "../eth.h"
#include "eth_intel_igc_priv.h"

#define DT_DRV_COMPAT intel_igc_mac

/**
 * @brief Amend the register value as per the mask and set/clear the bit.
 */
static void igc_modify(mm_reg_t base, uint32_t offset, uint32_t config, bool set)
{
	uint32_t val = sys_read32(base + offset);

	if (set) {
		val |= config;
	} else {
		val &= ~config;
	}

	sys_write32(val, base + offset);
}

/**
 * @brief Significant register changes required another register operation
 * to take effect. This status register read mimics that logic.
 */
static void igc_reg_refresh(mm_reg_t base)
{
	sys_read32(base + INTEL_IGC_STATUS);
}

/**
 * @brief Get the index of a specific transmit descriptor within the ring.
 * This getter also works for multiple queues.
 */
static uint16_t get_tx_desc_idx(const struct device *dev, union dma_tx_desc *current_desc,
				uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	union dma_tx_desc *tx_desc_base;

	tx_desc_base = (union dma_tx_desc *)&data->tx.desc[queue * cfg->num_tx_desc];

	return (current_desc - tx_desc_base);
}

/**
 * @brief Get the index of a specific receive descriptor within the ring.
 * This getter also works for multiple queues.
 */
static uint16_t get_rx_desc_idx(const struct device *dev, union dma_rx_desc *current_desc,
				uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	union dma_rx_desc *rx_desc_base;

	rx_desc_base = (union dma_rx_desc *)&data->rx.desc[queue * cfg->num_rx_desc];

	return (current_desc - rx_desc_base);
}

/**
 * @brief Check if the next descriptor is unavailable for DMA operation.
 */
static bool is_desc_unavailable(uint32_t next_desc_idx, uint32_t current_rd_ptr_idx)
{
	return (next_desc_idx == current_rd_ptr_idx);
}

/**
 * @brief Check if the DMA operation is complete by using the writeback.dd bit.
 */
static bool is_dma_done(bool dd)
{
	return dd;
}

/**
 * @brief This function retrieves the next available transmit descriptor from the ring
 * and ensures that the descriptor is available for DMA operation.
 */
static union dma_tx_desc *eth_intel_igc_get_tx_desc(const struct device *dev, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t current_wr_idx, next_wr_idx;
	union dma_tx_desc *desc;

	k_sem_take(&data->tx.sem[queue], K_FOREVER);

	current_wr_idx = data->tx.ring_wr_ptr[queue];
	next_wr_idx = (current_wr_idx + 1) % cfg->num_tx_desc;

	if (is_desc_unavailable(next_wr_idx, data->tx.ring_rd_ptr[queue])) {
		k_sem_give(&data->tx.sem[queue]);
		return NULL;
	}

	desc = data->tx.desc + (queue * cfg->num_tx_desc + current_wr_idx);
	data->tx.ring_wr_ptr[queue] = next_wr_idx;

	k_sem_give(&data->tx.sem[queue]);

	return desc;
}

/**
 * @brief This function checks if the DMA operation is complete by verifying the
 * writeback.dd bit. If the DMA operation is complete, it updates the read pointer
 * and clears the descriptor.
 */
static union dma_tx_desc *eth_intel_igc_release_tx_desc(const struct device *dev, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t current_rd_idx, next_rd_idx;
	union dma_tx_desc *desc;

	k_sem_take(&data->tx.sem[queue], K_FOREVER);

	current_rd_idx = data->tx.ring_rd_ptr[queue];
	desc = data->tx.desc + (queue * cfg->num_tx_desc + current_rd_idx);

	if (is_dma_done(desc->writeback.dd)) {
		next_rd_idx = (current_rd_idx + 1) % cfg->num_tx_desc;
		data->tx.ring_rd_ptr[queue] = next_rd_idx;
		memset((void *)desc, 0, sizeof(union dma_tx_desc));
	} else {
		desc = NULL;
	}

	k_sem_give(&data->tx.sem[queue]);

	return desc;
}

/**
 * @brief This function returns the total number of consumed transmit descriptors from
 * overall transmit descriptor ring of the mentioned queue.
 */
static uint32_t eth_intel_igc_completed_txdesc_num(const struct device *dev, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t rd_idx, count = 0;
	union dma_tx_desc *desc;

	rd_idx = data->tx.ring_rd_ptr[queue];
	while (rd_idx != data->tx.ring_wr_ptr[queue]) {
		desc = (data->tx.desc + (queue * cfg->num_tx_desc + rd_idx));
		if (!is_dma_done(desc->writeback.dd)) {
			break;
		}
		rd_idx = (rd_idx + 1) % cfg->num_tx_desc;
		count++;
	}

	return count;
}

/**
 * @brief This function retrieves the next available receive descriptor from the ring
 * and ensures that the descriptor is available for DMA operation.
 */
static union dma_rx_desc *eth_intel_igc_get_rx_desc(const struct device *dev, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t current_wr_idx, next_wr_idx;
	union dma_rx_desc *desc;

	k_sem_take(&data->rx.sem[queue], K_FOREVER);

	current_wr_idx = data->rx.ring_wr_ptr[queue];
	next_wr_idx = (current_wr_idx + 1) % cfg->num_rx_desc;

	if (is_desc_unavailable(next_wr_idx, data->rx.ring_rd_ptr[queue])) {
		k_sem_give(&data->rx.sem[queue]);
		return NULL;
	}

	desc = data->rx.desc + (queue * cfg->num_rx_desc + current_wr_idx);
	data->rx.ring_wr_ptr[queue] = next_wr_idx;

	k_sem_give(&data->rx.sem[queue]);

	return desc;
}

/**
 * @brief This function checks if the DMA operation is complete by verifying the
 * writeback.dd bit. If the DMA operation is complete, it updates the read pointer
 * and clears the descriptor.
 */
static union dma_rx_desc *eth_intel_igc_release_rx_desc(const struct device *dev, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t current_rd_idx, next_rd_idx;
	union dma_rx_desc *desc = NULL;

	k_sem_take(&data->rx.sem[queue], K_FOREVER);

	current_rd_idx = data->rx.ring_rd_ptr[queue];
	desc = data->rx.desc + (queue * cfg->num_rx_desc + current_rd_idx);

	if (is_dma_done(desc->writeback.dd)) {
		next_rd_idx = (current_rd_idx + 1) % cfg->num_rx_desc;
		data->rx.ring_rd_ptr[queue] = next_rd_idx;
	} else {
		desc = NULL;
	}

	k_sem_give(&data->rx.sem[queue]);

	return desc;
}

/**
 * @brief This function return total number of consumed receive descriptors from overall
 * receive descriptor ring of the mentioned queue.
 */
static uint32_t eth_intel_igc_completed_rxdesc_num(const struct device *dev, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	union dma_rx_desc *desc = NULL;
	uint32_t idx, count = 0;

	idx = data->rx.ring_rd_ptr[queue];
	while (idx != data->rx.ring_wr_ptr[queue]) {
		desc = (data->rx.desc + (queue * cfg->num_rx_desc + idx));
		if (!is_dma_done(desc->writeback.dd)) {
			break;
		}
		idx = (idx + 1) % cfg->num_rx_desc;
		count++;
	}

	return count;
}

/**
 * @brief Interrupt Service Routine for handling queue interrupts.
 */
static void eth_intel_igc_queue_isr(void *arg)
{
	const struct eth_intel_igc_intr_info *info = (struct eth_intel_igc_intr_info *)arg;
	const struct device *dev = info->mac;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint8_t msix = info->id;

	igc_modify(data->base, INTEL_IGC_EICS, BIT(msix), false);
	k_work_submit(&data->tx_work[msix]);
	k_work_schedule(&data->rx_work[msix], K_MSEC(0));

	sys_read32(data->base + INTEL_IGC_ICR);
	igc_modify(data->base, INTEL_IGC_EIMS, BIT(msix), true);
}

static int eth_intel_igc_connect_queue_msix_vector(pcie_bdf_t bdf, const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;

	for (uint8_t msix = 0; msix < cfg->num_queues; msix++) {
		struct eth_intel_igc_intr_info *info = &data->intr_info[msix];

		info->id = msix;
		info->mac = dev;
		if (!pcie_msi_vector_connect(bdf, &data->msi_vec[msix],
					     (void *)eth_intel_igc_queue_isr, info, 0)) {
			LOG_ERR("Failed to connect queue_%d interrupt handler", msix);
			return -EIO;
		}
	}

	return 0;
}

static int eth_intel_igc_pcie_msix_setup(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	pcie_bdf_t bdf;
	int ret;

	bdf = eth_intel_get_pcie_bdf(cfg->platform);
	if (bdf == PCIE_BDF_NONE) {
		LOG_ERR("Failed to get PCIe BDF");
		return -EINVAL;
	}

	ret = pcie_msi_vectors_allocate(bdf, CONFIG_ETH_INTEL_IGC_INT_PRIORITY, data->msi_vec,
					cfg->num_msix);
	if (ret < cfg->num_msix) {
		LOG_ERR("Failed to allocate MSI-X vectors");
		return -EIO;
	}

	ret = eth_intel_igc_connect_queue_msix_vector(bdf, dev);
	if (ret < 0) {
		return ret;
	}

	if (!pcie_msi_enable(bdf, data->msi_vec, cfg->num_msix, 0)) {
		LOG_ERR("Failed to enable MSI-X vectors");
		return -EIO;
	}

	return 0;
}

/**
 * @brief This function maps the IGC device interrupts order to MSI-X vectors.
 */
static void eth_intel_igc_map_intr_to_vector(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t config, reg_idx;

	/* Set MSI-X capability */
	config = INTEL_IGC_GPIE_NSICR | INTEL_IGC_GPIE_MSIX_MODE | INTEL_IGC_GPIE_EIAME |
		 INTEL_IGC_GPIE_PBA;
	igc_modify(data->base, INTEL_IGC_GPIE, config, true);

	/* Configure IVAR RX and TX for each queue interrupt */
	for (uint8_t msix = 0; msix < cfg->num_queues; msix++) {
		reg_idx = msix >> 1;
		config = sys_read32(data->base + (INTEL_IGC_IVAR_BASE_ADDR + (reg_idx << 2)));

		if (msix % 2) {
			config &= INTEL_IGC_IVAR_MSI_CLEAR_TX1_TX3;
			config |= (msix | INTEL_IGC_IVAR_INT_VALID_BIT) << 24;
			config &= INTEL_IGC_IVAR_MSI_CLEAR_RX1_RX3;
			config |= (msix | INTEL_IGC_IVAR_INT_VALID_BIT) << 16;
		} else {
			config &= INTEL_IGC_IVAR_MSI_CLEAR_TX0_TX2;
			config |= (msix | INTEL_IGC_IVAR_INT_VALID_BIT) << 8;
			config &= INTEL_IGC_IVAR_MSI_CLEAR_RX0_RX2;
			config |= (msix | INTEL_IGC_IVAR_INT_VALID_BIT);
		}

		sys_write32(config, data->base + (INTEL_IGC_IVAR_BASE_ADDR + (reg_idx << 2)));
	}
}

/**
 * @brief Helper function to write MAC address to RAL and RAH registers.
 */
static void eth_intel_igc_set_mac_addr(mm_reg_t base, int index, const uint8_t *mac_addr,
				       uint32_t rah_config)
{
	uint32_t mac_addr_hi = (mac_addr[5] << 8) | mac_addr[4] | rah_config;
	uint32_t mac_addr_lo =
		(mac_addr[3] << 24) | (mac_addr[2] << 16) | (mac_addr[1] << 8) | mac_addr[0];

	sys_write32(mac_addr_hi, base + INTEL_IGC_RAH(index));
	igc_reg_refresh(base);
	sys_write32(mac_addr_lo, base + INTEL_IGC_RAL(index));
	igc_reg_refresh(base);
}

/**
 * @brief This function configures the MAC address filtering for the IGC, allowing it
 * to filter packets based on the MAC address and filter mode.
 */
static void eth_intel_igc_set_mac_filter(const struct device *dev,
					 enum eth_igc_mac_filter_mode mode, const uint8_t *mac_addr,
					 int index, uint8_t queue)
{
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t config = 0;

	/* Queue number Settings */
	config &= ~RAH_QSEL_MASK;
	config |= ((queue << RAH_QSEL_SHIFT) | RAH_QSEL_ENABLE);

	if (mode == SRC_ADDR) {
		config |= (config & ~RAH_ASEL_MASK) | RAH_ASEL_SRC_ADDR;
	}

	config |= INTEL_IGC_RAH_AV;
	eth_intel_igc_set_mac_addr(data->base, index, mac_addr, config);
}

static void eth_intel_igc_phylink_cb(const struct device *phy, struct phy_link_state *state,
				     void *user_data)
{
	struct eth_intel_igc_mac_data *data = (struct eth_intel_igc_mac_data *)user_data;

	if (state->is_up) {
		net_eth_carrier_on(data->iface);
	} else {
		net_eth_carrier_off(data->iface);
	}
}

static void eth_intel_igc_intr_enable(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t config = 0;

	/* Clear pending interrupt */
	sys_read32(data->base + INTEL_IGC_ICR);

	/* Prepare bit mask of Queue interrupt */
	for (uint8_t msix = 0; msix < cfg->num_queues; msix++) {
		config |= BIT(msix);
	}

	sys_write32(config, data->base + INTEL_IGC_EIAC);
	igc_modify(data->base, INTEL_IGC_EIAM, config, true);
	sys_write32(config, data->base + INTEL_IGC_EIMS);
	igc_reg_refresh(data->base);
}

static void eth_intel_igc_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;

	/* Set interface */
	data->iface = iface;

	/* Initialize ethernet L2 */
	ethernet_init(iface);

	/* Set MAC address */
	if (net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				 NET_LINK_ETHERNET) < 0) {
		LOG_ERR("Failed to set mac address");
		return;
	}

	eth_intel_igc_set_mac_filter(dev, DEST_ADDR, data->mac_addr, 0, 0);

	if (device_is_ready(cfg->phy)) {
		phy_link_callback_set(cfg->phy, eth_intel_igc_phylink_cb, (void *)data);
	} else {
		LOG_ERR("PHY device is not ready");
		return;
	}

	eth_intel_igc_intr_enable(dev);
}

static enum ethernet_hw_caps eth_intel_igc_get_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_LINK_1000BASE;
}

static int eth_intel_igc_set_config(const struct device *dev, enum ethernet_config_type type,
				    const struct ethernet_config *eth_cfg)
{
	struct eth_intel_igc_mac_data *data = dev->data;

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		memcpy(data->mac_addr, eth_cfg->mac_address.addr, sizeof(eth_cfg->mac_address));
		net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				     NET_LINK_ETHERNET);
		eth_intel_igc_set_mac_filter(dev, DEST_ADDR, data->mac_addr, 0, 0);
		return 0;
	}

	return -ENOTSUP;
}

static const struct device *eth_intel_igc_get_phy(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;

	return cfg->phy;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_intel_igc_get_stats(const struct device *dev)
{
	struct eth_intel_igc_mac_data *data = dev->data;
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct net_stats_eth *stats = &data->stats;

	stats->bytes.sent += sys_read32(data->base + INTEL_IGC_TOTL);
	stats->bytes.received += sys_read32(data->base + INTEL_IGC_TORL);
	stats->pkts.tx += sys_read32(data->base + INTEL_IGC_TPT);
	stats->pkts.rx += sys_read32(data->base + INTEL_IGC_TPR);
	stats->broadcast.tx += sys_read32(data->base + INTEL_IGC_BPTC);
	stats->broadcast.rx += sys_read32(data->base + INTEL_IGC_BPRC);
	stats->multicast.tx += sys_read32(data->base + INTEL_IGC_MPTC);
	stats->multicast.rx += sys_read32(data->base + INTEL_IGC_MPRC);
	stats->errors.rx += sys_read32(data->base + INTEL_IGC_RERC);
	stats->error_details.rx_length_errors += sys_read32(data->base + INTEL_IGC_RLEC);
	stats->error_details.rx_crc_errors += sys_read32(data->base + INTEL_IGC_CRCERRS);
	stats->error_details.rx_frame_errors += sys_read32(data->base + INTEL_IGC_RJC);
	stats->error_details.rx_no_buffer_count += sys_read32(data->base + INTEL_IGC_RNBC);
	stats->error_details.rx_long_length_errors += sys_read32(data->base + INTEL_IGC_ROC);
	stats->error_details.rx_short_length_errors += sys_read32(data->base + INTEL_IGC_RUC);
	stats->error_details.rx_align_errors += sys_read32(data->base + INTEL_IGC_ALGNERRC);
	stats->error_details.rx_buf_alloc_failed += sys_read32(data->base + INTEL_IGC_MPC);
	stats->error_details.tx_aborted_errors += sys_read32(data->base + INTEL_IGC_DC);
	stats->flow_control.rx_flow_control_xon += sys_read32(data->base + INTEL_IGC_XONRXC);
	stats->flow_control.rx_flow_control_xoff += sys_read32(data->base + INTEL_IGC_XOFFRXC);
	stats->flow_control.tx_flow_control_xon += sys_read32(data->base + INTEL_IGC_XONTXC);
	stats->flow_control.tx_flow_control_xoff += sys_read32(data->base + INTEL_IGC_XOFFTXC);
	stats->collisions += sys_read32(data->base + INTEL_IGC_COLC);
	for (uint8_t queue = 0; queue < cfg->num_queues; queue++) {
		stats->tx_dropped += sys_read32(data->base + INTEL_IGC_TQDPC(queue));
	}

	return &data->stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

/**
 * @brief This function releases completed transmit descriptors, cleans up the associated
 * net_buf and net_pkt, and frees any allocated resources.
 */
static void eth_intel_igc_tx_clean(struct eth_intel_igc_mac_data *data, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = data->mac->config;
	uint32_t idx = 0, clean_count;
	union dma_tx_desc *desc;

	clean_count = eth_intel_igc_completed_txdesc_num(data->mac, queue);
	for (uint8_t count = 0; count < clean_count; count++) {
		desc = eth_intel_igc_release_tx_desc(data->mac, queue);
		if (desc == NULL) {
			LOG_ERR("No more transmit descriptor available to release");
			continue;
		}

		idx = get_tx_desc_idx(data->mac, desc, queue);
		net_pkt_frag_unref(*(data->tx.frag + queue * cfg->num_tx_desc + idx));
		net_pkt_unref(*(data->tx.pkt + queue * cfg->num_tx_desc + idx));
	}
}

/**
 * @brief This function retrieves the next available transmit descriptor from the ring,
 * sets up the descriptor with the fragment data, and updates the write pointer.
 */
static int eth_intel_igc_tx_frag(const struct device *dev, struct net_pkt *pkt,
				 struct net_buf *frag, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint16_t total_len = net_pkt_get_len(pkt);
	union dma_tx_desc *desc;
	uint32_t idx = 0;

	desc = eth_intel_igc_get_tx_desc(dev, queue);
	if (desc == NULL) {
		LOG_ERR("No more transmit descriptors available");
		return -ENOMEM;
	}

	idx = get_tx_desc_idx(dev, desc, queue);
	/* Store the pkt and header frag, then clear it during transmit clean. */
	*(data->tx.frag + queue * cfg->num_tx_desc + idx) = frag->frags ? 0 : pkt->frags;
	*(data->tx.pkt + queue * cfg->num_tx_desc + idx) = frag->frags ? 0 : pkt;

	desc->read.data_buf_addr = (uint64_t)sys_cpu_to_le64((uint64_t)frag->data);
	/* Copy the total payload length */
	desc->read.payloadlen = total_len;
	/* Copy the particular frag's buffer length */
	desc->read.data_len = frag->len;
	desc->read.desc_type = INTEL_IGC_TX_DESC_TYPE;
	desc->read.ifcs = true;
	desc->read.dext = true;

	/* DMA engine processes the entire packet as a single unit */
	if (frag->frags == NULL) {
		desc->read.eop = true;
		desc->read.rs = true;

		sys_write32(data->tx.ring_wr_ptr[queue], data->base + INTEL_IGC_TDT(queue));
	}

	return 0;
}

/**
 * @brief This function handles the network packet transmission by processing each
 * fragmented frames and sending it through appropriate queue.
 */
static int eth_intel_igc_tx_data(const struct device *dev, struct net_pkt *pkt)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint8_t queue = 0;
	int ret = 0;

	if (!net_if_is_up(data->iface)) {
		LOG_ERR("Ethernet interface is down");
		return -ENETDOWN;
	}

	/* Hold the packet until transmit clean done. */
	net_pkt_ref(pkt);

	queue = net_tx_priority2tc(net_pkt_priority(pkt));
	if (queue >= cfg->num_queues) {
		queue = cfg->num_queues - 1;
	}

	for (struct net_buf *frag = pkt->frags; frag; frag = frag->frags) {
		/* Hold the Header fragment until transmit clean done */
		if (frag == pkt->frags) {
			net_pkt_frag_ref(frag);
		}

		ret = eth_intel_igc_tx_frag(dev, pkt, frag, queue);
		if (ret < 0) {
			LOG_ERR("Failed to transmit in queue number: %d", queue);
		}
	}

	return ret;
}

/**
 * @brief Identify the address family of received packets as per header type.
 */
static sa_family_t eth_intel_igc_get_sa_family(uint8_t *rx_buf)
{
	struct net_eth_hdr *eth_hdr = (struct net_eth_hdr *)rx_buf;

	switch (ntohs(eth_hdr->type)) {
	case NET_ETH_PTYPE_IP:
		return AF_INET;
	case NET_ETH_PTYPE_IPV6:
		return AF_INET6;
	default:
		break;
	}

	return AF_UNSPEC;
}

/**
 * @brief This function updates the tail pointer of the RX descriptor ring, retrieves the
 * next available RX descriptor, and prepare it for receiving incoming packets by setting
 * the packet buffer address.
 */
static void eth_intel_igc_rx_data_hdl(struct eth_intel_igc_mac_data *data, uint8_t queue,
				      uint32_t idx, union dma_rx_desc *desc)
{
	const struct eth_intel_igc_mac_cfg *cfg = data->mac->config;

	sys_write32(idx, data->base + INTEL_IGC_RDT(queue));
	desc = eth_intel_igc_get_rx_desc(data->mac, queue);
	if (desc == NULL) {
		LOG_ERR("No more rx descriptor available");
		return;
	}

	/* Find descriptor position and prepare it for next DMA cycle */
	idx = get_rx_desc_idx(data->mac, desc, queue);
	desc->read.pkt_buf_addr = (uint64_t)sys_cpu_to_le64(
		(uint64_t)(data->rx.buf + (queue * cfg->num_rx_desc + idx) * ETH_MAX_FRAME_SZ));
	desc->writeback.dd = 0;
}

static void eth_intel_igc_rx_data_hdl_err(struct eth_intel_igc_mac_data *data, uint8_t queue,
					  uint32_t idx, union dma_rx_desc *desc,
					  struct net_pkt *pkt)
{
	net_pkt_unref(pkt);
	eth_intel_igc_rx_data_hdl(data, queue, idx, desc);
}

/**
 * @brief This function retrieves completed receive descriptors, allocates net_pkt
 * buffers, copies the received data into the buffers, and submits the packets to
 * the network stack.
 */
static void eth_intel_igc_rx_data(struct eth_intel_igc_mac_data *data, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = data->mac->config;
	uint32_t count, idx = 0, completed_count = 0;
	uint8_t *rx_buf;
	union dma_rx_desc *desc;
	struct net_pkt *pkt;
	int ret;

	completed_count = eth_intel_igc_completed_rxdesc_num(data->mac, queue);
	for (count = 0; count < completed_count; count++) {
		/* Retrieve the position of next descriptor to be processed */
		idx = data->rx.ring_rd_ptr[queue];
		desc = eth_intel_igc_release_rx_desc(data->mac, queue);
		if (desc == NULL) {
			LOG_ERR("RX descriptor is NULL");
			continue;
		}

		if (!net_if_is_up(data->iface) || !desc->writeback.pkt_len) {
			LOG_ERR("RX interface is down or pkt_len is %d", desc->writeback.pkt_len);
			eth_intel_igc_rx_data_hdl_err(data, queue, idx, desc, NULL);
			continue;
		}

		/* Get the DMA buffer pointer as per index */
		rx_buf = data->rx.buf + ((queue * cfg->num_rx_desc + idx) * ETH_MAX_FRAME_SZ);

		/* Allocate packet buffer as per address family */
		pkt = net_pkt_rx_alloc_with_buffer(data->iface, desc->writeback.pkt_len,
						   eth_intel_igc_get_sa_family(rx_buf), 0,
						   K_MSEC(200));
		if (pkt == NULL) {
			LOG_ERR("Failed to allocate Receive buffer");
			eth_intel_igc_rx_data_hdl_err(data, queue, idx, desc, NULL);
			continue;
		}

		/* Write DMA buffer to packet */
		ret = net_pkt_write(pkt, (void *)rx_buf, desc->writeback.pkt_len);
		if (ret < 0) {
			LOG_ERR("Failed to write Receive buffer to packet");
			eth_intel_igc_rx_data_hdl_err(data, queue, idx, desc, pkt);
			continue;
		}

		/* Process received packet */
		ret = net_recv_data(data->iface, pkt);
		if (ret < 0) {
			LOG_ERR("Failed to enqueue the Receive packet: %d", queue);
			eth_intel_igc_rx_data_hdl_err(data, queue, idx, desc, pkt);
			continue;
		}

		eth_intel_igc_rx_data_hdl(data, queue, idx, desc);
	}
}

/**
 * @brief Configure and Enable the Transmit Control Register.
 */
static void eth_intel_igc_tctl_setup(mm_reg_t tctl)
{
	uint32_t config = sys_read32(tctl);

	config &= ~INTEL_IGC_TCTL_CT;
	config |= INTEL_IGC_TCTL_EN | INTEL_IGC_TCTL_PSP | INTEL_IGC_TCTL_RTLC |
		  INTEL_IGC_COLLISION_THRESHOLD << INTEL_IGC_TCTL_CT_SHIFT;

	sys_write32(config, tctl);
}

/**
 * @brief IGC expects the DMA descriptors to be aligned to 128-byte boundaries for efficient
 * access and to avoid potential data corruption or performance issues.
 */
static void *eth_intel_igc_aligned_alloc(uint32_t size)
{
	void *desc_base = k_aligned_alloc(4096, size);

	if (desc_base == NULL) {
		return NULL;
	}
	memset(desc_base, 0, size);

	return desc_base;
}

/**
 * @brief This function initializes the transmit DMA descriptor ring for all queues.
 */
static void eth_intel_igc_init_tx_desc_ring(const struct eth_intel_igc_mac_cfg *cfg,
					    struct eth_intel_igc_mac_data *data)
{
	uint64_t desc_addr;
	uint32_t config;

	for (uint8_t queue = 0; queue < cfg->num_queues; queue++) {
		k_sem_init(&data->tx.sem[queue], 1, 1);

		/* Disable the transmit descriptor ring */
		sys_write32(0, data->base + INTEL_IGC_TXDCTL(queue));
		igc_reg_refresh(data->base);

		/* Program the transmit descriptor ring total length */
		sys_write32(cfg->num_tx_desc * sizeof(union dma_tx_desc),
			    data->base + INTEL_IGC_TDLEN(queue));

		/* Program the descriptor base address */
		desc_addr = (uint64_t)(data->tx.desc + (queue * cfg->num_tx_desc));
		sys_write32((uint32_t)(desc_addr >> 32), data->base + INTEL_IGC_TDBAH(queue));
		sys_write32((uint32_t)desc_addr, data->base + INTEL_IGC_TDBAL(queue));

		/* Reset Head and Tail Descriptor Pointers */
		sys_write32(0, data->base + INTEL_IGC_TDH(queue));
		sys_write32(0, data->base + INTEL_IGC_TDT(queue));

		/* Configure TX DMA interrupt trigger thresholds */
		config = INTEL_IGC_TX_PTHRESH_VAL << INTEL_IGC_TX_PTHRESH_SHIFT |
			 INTEL_IGC_TX_HTHRESH_VAL << INTEL_IGC_TX_HTHRESH_SHIFT |
			 INTEL_IGC_TX_WTHRESH_VAL << INTEL_IGC_TX_WTHRESH_SHIFT;
		/* Enable the transmit descriptor ring */
		config |= INTEL_IGC_TXDCTL_QUEUE_ENABLE;
		sys_write32(config, data->base + INTEL_IGC_TXDCTL(queue));
	}
}

/**
 * @brief This function initializes the transmit DMA descriptor ring.
 * It sets up the descriptor base addresses, lengths, and control registers.
 */
static int eth_intel_igc_tx_dma_init(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t size;

	/* Calculate the total size of the TX descriptor buffer */
	size = cfg->num_queues * cfg->num_tx_desc * sizeof(union dma_tx_desc);
	/* Allocate memory for the TX descriptor buffer */
	size = ROUND_UP(size, sizeof(union dma_tx_desc));
	data->tx.desc = (union dma_tx_desc *)eth_intel_igc_aligned_alloc(size);
	if (data->tx.desc == NULL) {
		LOG_ERR("Transmit descriptor buffer alloc failed");
		return -ENOBUFS;
	}

	eth_intel_igc_init_tx_desc_ring(cfg, data);

	/* Configure internal transmit descriptor buffer size */
	sys_write32(INTEL_IGC_TXPBS_TXPBSIZE_DEFAULT, data->base + INTEL_IGC_TXPBS);
	eth_intel_igc_tctl_setup(data->base + INTEL_IGC_TCTL);

	return 0;
}

/**
 * @brief Configure and Enable the Receive Control Register.
 */
static void eth_intel_igc_rctl_setup(mm_reg_t rctl)
{
	uint32_t config = sys_read32(rctl);

	/* Multicast / Unicast Table Offset */
	config &= ~(0x3 << INTEL_IGC_RCTL_MO_SHIFT);
	/* Do not store bad packets */
	config &= ~INTEL_IGC_RCTL_SBP;
	/* Turn off VLAN filters */
	config &= ~INTEL_IGC_RCTL_VFE;
	config |= INTEL_IGC_RCTL_EN | INTEL_IGC_RCTL_BAM |
		  /* Strip the CRC */
		  INTEL_IGC_RCTL_SECRC | INTEL_IGC_RCTL_SZ_2048;

	sys_write32(config, rctl);
}

/**
 * @brief This function ensures that each Receive DMA descriptor is populated with
 * a pre-allocated packet buffer, enabling the device to receive and store incoming
 * packets efficiently.
 */
static int eth_intel_igc_rx_desc_prepare(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	union dma_rx_desc *desc;
	uint8_t *buf;

	/* Allocate memory for receive DMA buffer */
	data->rx.buf = (uint8_t *)k_calloc(cfg->num_queues * cfg->num_rx_desc, ETH_MAX_FRAME_SZ);
	if (data->rx.buf == NULL) {
		LOG_ERR("Receive DMA buffer alloc failed");
		return -ENOBUFS;
	}

	/* Assign allocated memory to each descriptor as per it's index */
	for (uint8_t queue = 0; queue < cfg->num_queues; queue++) {
		desc = &data->rx.desc[queue * cfg->num_rx_desc];

		for (uint32_t desc_idx = 0; desc_idx < cfg->num_rx_desc; desc_idx++) {
			/* Set the pkt buffer address */
			buf = data->rx.buf +
			      ((queue * cfg->num_rx_desc + desc_idx) * ETH_MAX_FRAME_SZ);

			desc[desc_idx].read.pkt_buf_addr = (uint64_t)sys_cpu_to_le64((uint64_t)buf);
			desc[desc_idx].read.hdr_buf_addr = (uint64_t)&desc[desc_idx];
		}

		/* Update tail pointer in hardware and copy the same for driver reference */
		sys_write32(cfg->num_rx_desc - 1, data->base + INTEL_IGC_RDT(queue));
		data->rx.ring_wr_ptr[queue] = cfg->num_rx_desc - 1;
	}

	return 0;
}

/**
 * @brief This function initializes the receive DMA descriptor ring for all queues.
 */
static void eth_intel_igc_init_rx_desc_ring(const struct eth_intel_igc_mac_cfg *cfg,
					    struct eth_intel_igc_mac_data *data)
{
	uint64_t desc_addr;
	uint32_t config;

	for (uint8_t queue = 0; queue < cfg->num_queues; queue++) {
		k_sem_init(&data->rx.sem[queue], 1, 1);

		/* Disable the receive descriptor ring */
		sys_write32(0, data->base + INTEL_IGC_RXDCTL(queue));
		igc_reg_refresh(data->base);

		/* Program the receive descriptor ring total length */
		sys_write32(cfg->num_rx_desc * sizeof(union dma_rx_desc),
			    data->base + INTEL_IGC_RDLEN(queue));

		/* Program the descriptor base address */
		desc_addr = (uint64_t)(data->rx.desc + (queue * cfg->num_rx_desc));
		sys_write32((uint32_t)(desc_addr >> 32), data->base + INTEL_IGC_RDBAH(queue));
		sys_write32((uint32_t)desc_addr, data->base + INTEL_IGC_RDBAL(queue));

		/* Configure the receive descriptor control */
		config = INTEL_IGC_SRRCTL_BSIZEPKT(ETH_MAX_FRAME_SZ);
		config |= INTEL_IGC_SRRCTL_BSIZEHDR(INTEL_IGC_RXBUFFER_256);
		config |= INTEL_IGC_SRRCTL_DESCTYPE_ADV_ONEBUF;
		config |= INTEL_IGC_SRRCTL_DROP_EN;
		sys_write32(config, data->base + INTEL_IGC_SRRCTL(queue));

		/* Reset Head and Tail Descriptor Pointers */
		sys_write32(0, data->base + INTEL_IGC_RDH(queue));
		sys_write32(0, data->base + INTEL_IGC_RDT(queue));

		config = sys_read32(data->base + INTEL_IGC_RXDCTL(queue));
		config &= INTEL_IGC_RX_THRESH_RESET;
		/* Configure RX DMA interrupt trigger thresholds */
		config |= INTEL_IGC_RX_PTHRESH_VAL << INTEL_IGC_RX_PTHRESH_SHIFT |
			  INTEL_IGC_RX_HTHRESH_VAL << INTEL_IGC_RX_HTHRESH_SHIFT |
			  INTEL_IGC_RX_WTHRESH_VAL << INTEL_IGC_RX_WTHRESH_SHIFT;
		/* Enable the receive descriptor ring */
		config |= INTEL_IGC_RXDCTL_QUEUE_ENABLE;
		sys_write32(config, data->base + INTEL_IGC_RXDCTL(queue));
		igc_reg_refresh(data->base);
	}
}

/**
 * @brief This function initializes the receive descriptor ring.
 * It sets up the descriptor base address, length, and control registers.
 */
static int eth_intel_igc_rx_dma_init(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t size;
	int ret;

	/*
	 * TODO: enable RSS mapping.
	 * TODO: enable interrupt moderation here.
	 * TODO: enable Checksum offload here.
	 * TODO: enable VLAN offload here.
	 */

	/* Disable receive logic until descriptor setup */
	igc_modify(data->base, INTEL_IGC_RCTL, INTEL_IGC_RCTL_EN, false);
	sys_write32(0, data->base + INTEL_IGC_RXCSUM);

	/* Calculate the total size of the RX descriptor buffer */
	size = cfg->num_queues * cfg->num_rx_desc * sizeof(union dma_rx_desc);
	size = ROUND_UP(size, sizeof(union dma_rx_desc));

	/* Allocate memory for the RX descriptor buffer */
	data->rx.desc = (union dma_rx_desc *)eth_intel_igc_aligned_alloc(size);
	if (data->rx.desc == NULL) {
		LOG_ERR("Receive descriptor buffer alloc failed");
		return -ENOBUFS;
	}

	eth_intel_igc_init_rx_desc_ring(cfg, data);

	/* Configure internal receive descriptor buffer size */
	sys_write32(INTEL_IGC_RXPBS_RXPBSIZE_DEFAULT, data->base + INTEL_IGC_RXPBS);

	ret = eth_intel_igc_rx_desc_prepare(dev);
	if (ret < 0) {
		LOG_ERR("Receive descriptor prepare failed");
		return ret;
	}
	eth_intel_igc_rctl_setup(data->base + INTEL_IGC_RCTL);

	return ret;
}

/**
 * @brief This function validates the MAC address and returns true on success.
 */
static bool eth_intel_igc_is_valid_mac_addr(uint8_t *mac_addr)
{
	if (UNALIGNED_GET((uint32_t *)mac_addr) == INTEL_IGC_DEF_MAC_ADDR) {
		return false;
	}

	if (UNALIGNED_GET((uint32_t *)(mac_addr + 0)) == 0x00000000 &&
	    UNALIGNED_GET((uint16_t *)(mac_addr + 4)) == 0x0000) {
		LOG_DBG("Invalid Mac Address");
		return false;
	}

	if (mac_addr[0] & 0x01) {
		LOG_DBG("Multicast MAC address");
		return false;
	}

	return true;
}

/* @brief When the device is configured to use MAC address from EEPROM, i226 firmware
 * will populate both RAL and RAH with the user provided MAC address.
 */
static void eth_intel_igc_get_preloaded_mac_addr(mm_reg_t base, uint8_t *mac_addr)
{
	uint32_t mac_addr_hi, mac_addr_lo;

	mac_addr_lo = sys_read32(base + INTEL_IGC_RAL(0));
	mac_addr_hi = sys_read32(base + INTEL_IGC_RAH(0));

	mac_addr[0] = (uint8_t)(mac_addr_lo & 0xFF);
	mac_addr[1] = (uint8_t)((mac_addr_lo >> 8) & 0xFF);
	mac_addr[2] = (uint8_t)((mac_addr_lo >> 16) & 0xFF);
	mac_addr[3] = (uint8_t)((mac_addr_lo >> 24) & 0xFF);

	mac_addr[4] = (uint8_t)(mac_addr_hi & 0xFF);
	mac_addr[5] = (uint8_t)((mac_addr_hi >> 8) & 0xFF);
}

static void eth_intel_igc_get_mac_addr(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;

	if (cfg->random_mac_address) {
		LOG_INF("Assign Random MAC address");
		gen_random_mac(data->mac_addr, 0, 0xA0, 0xC9);
		return;
	}

	if (eth_intel_igc_is_valid_mac_addr(data->mac_addr)) {
		LOG_INF("Assign MAC address from Device Tree");
		return;
	}

	eth_intel_igc_get_preloaded_mac_addr(data->base, data->mac_addr);
	if (eth_intel_igc_is_valid_mac_addr(data->mac_addr)) {
		LOG_INF("Assign MAC address from EEPROM");
		return;
	}
}

static void eth_intel_igc_rx_addrs_init(const struct device *dev)
{
	struct eth_intel_igc_mac_data *data = dev->data;
	uint8_t reset_addr[NET_ETH_ADDR_LEN] = {0};
	uint16_t rar_count = 128;

	eth_intel_igc_get_mac_addr(dev);
	LOG_INF("IGC MAC addr %02X:%02X:%02X:%02X:%02X:%02X", data->mac_addr[0], data->mac_addr[1],
		data->mac_addr[2], data->mac_addr[3], data->mac_addr[4], data->mac_addr[5]);

	/* Program valid MAC address in index 0 */
	eth_intel_igc_set_mac_addr(data->base, 0, data->mac_addr, INTEL_IGC_RAH_AV);

	/* Program reset_addr to unused rar index */
	for (uint8_t rar = 1; rar < rar_count; rar++) {
		eth_intel_igc_set_mac_addr(data->base, rar, reset_addr, INTEL_IGC_RAH_AV);
	}
}

/**
 * @brief This function disables the PCIe master access to the device, ensuring that
 * the device is ready to be controlled by the driver.
 */
static int eth_intel_igc_disable_pcie_master(mm_reg_t base)
{
	uint32_t timeout = INTEL_IGC_GIO_MASTER_DISABLE_TIMEOUT;
	mm_reg_t igc_stat = base + INTEL_IGC_STATUS;

	igc_modify(base, INTEL_IGC_CTRL, INTEL_IGC_CTRL_GIO_MASTER_DISABLE, true);
	/* Wait for the INTEL_IGC_STATUS_GIO_MASTER_ENABLE bit to clear */
	if (WAIT_FOR((sys_read32(igc_stat) & INTEL_IGC_STATUS_GIO_MASTER_ENABLE) == 0, timeout,
		     k_msleep(1))) {
		return 0;
	}

	LOG_ERR("Timeout waiting for GIO Master Request to complete");
	return -ETIMEDOUT;
}

static void eth_intel_igc_init_speed(struct eth_intel_igc_mac_data *data)
{
	mm_reg_t base = data->base;

	igc_modify(base, INTEL_IGC_CTRL, INTEL_IGC_CTRL_FRCSPD | INTEL_IGC_CTRL_FRCDPX, false);
	igc_modify(base, INTEL_IGC_CTRL, INTEL_IGC_CTRL_SLU, true);
}

static void eth_intel_igc_get_dev_ownership(struct eth_intel_igc_mac_data *data)
{
	igc_modify(data->base, INTEL_IGC_CTRL_EXT, INTEL_IGC_CTRL_EXT_DRV_LOAD, true);
}

static int eth_intel_igc_init_mac_hw(const struct device *dev)
{
	struct eth_intel_igc_mac_data *data = dev->data;
	int ret = 0;

	ret = eth_intel_igc_disable_pcie_master(data->base);
	if (ret < 0) {
		return ret;
	}

	sys_write32(UINT32_MAX, data->base + INTEL_IGC_IMC);
	sys_write32(0, data->base + INTEL_IGC_RCTL);
	sys_write32(INTEL_IGC_TCTL_PSP, data->base + INTEL_IGC_TCTL);
	igc_reg_refresh(data->base);

	/* MAC Reset */
	igc_modify(data->base, INTEL_IGC_CTRL, INTEL_IGC_CTRL_DEV_RST, true);
	k_msleep(INTEL_IGC_RESET_DELAY);

	/* MAC receive address Init */
	eth_intel_igc_rx_addrs_init(dev);

	eth_intel_igc_get_dev_ownership(data);
	eth_intel_igc_map_intr_to_vector(dev);
	eth_intel_igc_init_speed(data);

	return ret;
}

static int eth_intel_igc_init(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	int ret = 0;

	data->mac = dev;
	data->base = DEVICE_MMIO_GET(cfg->platform);
	if (!data->base) {
		LOG_ERR("Failed to get MMIO base address");
		return -ENODEV;
	}

	ret = eth_intel_igc_init_mac_hw(dev);
	if (ret < 0) {
		return ret;
	}

	ret = eth_intel_igc_pcie_msix_setup(dev);
	if (ret < 0) {
		return ret;
	}

	ret = eth_intel_igc_tx_dma_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = eth_intel_igc_rx_dma_init(dev);
	if (ret < 0) {
		return ret;
	}

	cfg->config_func(dev);

	return 0;
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_intel_igc_iface_init,
	.get_capabilities = eth_intel_igc_get_caps,
	.set_config = eth_intel_igc_set_config,
	.send = eth_intel_igc_tx_data,
	.get_phy = eth_intel_igc_get_phy,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_intel_igc_get_stats,
#endif
};

#define NUM_QUEUES(n)  DT_INST_PROP(n, num_queues)
#define NUM_TX_DESC(n) DT_INST_PROP(n, num_tx_desc)
#define NUM_RX_DESC(n) DT_INST_PROP(n, num_rx_desc)
#define NUM_MSIX(n)    NUM_QUEUES(n) + ETH_IGC_NUM_MISC

/**
 * @brief Generate TX and RX interrupt handling functions as per queue.
 */
#define INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, queue)                                                   \
	static void eth_tx_irq_queue##n##_##queue(struct k_work *work)                             \
	{                                                                                          \
		struct eth_intel_igc_mac_data *data =                                              \
			CONTAINER_OF(work, struct eth_intel_igc_mac_data, tx_work[queue]);         \
		eth_intel_igc_tx_clean(data, queue);                                               \
	}                                                                                          \
	static void eth_rx_irq_queue##n##_##queue(struct k_work *work)                             \
	{                                                                                          \
		struct k_work_delayable *dwork = k_work_delayable_from_work(work);                 \
		struct eth_intel_igc_mac_data *data =                                              \
			CONTAINER_OF(dwork, struct eth_intel_igc_mac_data, rx_work[queue]);        \
		eth_intel_igc_rx_data(data, queue);                                                \
	}

#define INTEL_IGC_SETUP_QUEUE_WORK(n)                                                              \
	INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, 3)                                                       \
	INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, 2)                                                       \
	INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, 1)                                                       \
	INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, 0)

#define INTEL_IGC_INIT_QUEUE_WORK_EXP(n, queue)                                                    \
	k_work_init(&data->tx_work[queue], eth_tx_irq_queue##n##_##queue);                         \
	k_work_init_delayable(&data->rx_work[queue], eth_rx_irq_queue##n##_##queue);

/**
 * @brief Initialize deferred work for each hardware queue.
 */
#define INTEL_IGC_MAC_CONFIG_IRQ(n)                                                                \
	static void eth##n##_cfg_irq(const struct device *dev)                                     \
	{                                                                                          \
		struct eth_intel_igc_mac_data *data = dev->data;                                   \
		uint8_t queue = NUM_QUEUES(n);                                                     \
		if (queue > 3) {                                                                   \
			INTEL_IGC_INIT_QUEUE_WORK_EXP(n, 3);                                       \
		}                                                                                  \
		if (queue > 2) {                                                                   \
			INTEL_IGC_INIT_QUEUE_WORK_EXP(n, 2);                                       \
		}                                                                                  \
		if (queue > 1) {                                                                   \
			INTEL_IGC_INIT_QUEUE_WORK_EXP(n, 1);                                       \
		}                                                                                  \
		if (queue > 0) {                                                                   \
			INTEL_IGC_INIT_QUEUE_WORK_EXP(n, 0);                                       \
		}                                                                                  \
	}

/**
 * @brief Allocate various global objects required for managing tx and rx operations.
 */
#define INTEL_IGC_ALLOC_GLOBAL_OBJECTS(n)                                                          \
	struct k_sem tx_ring_lock_##n[NUM_QUEUES(n)];                                              \
	struct k_sem rx_ring_lock_##n[NUM_QUEUES(n)];                                              \
	static unsigned int tx_ring_wr_ptr_##n[NUM_QUEUES(n)];                                     \
	static unsigned int rx_ring_wr_ptr_##n[NUM_QUEUES(n)];                                     \
	static unsigned int tx_ring_rd_ptr_##n[NUM_QUEUES(n)];                                     \
	static unsigned int rx_ring_rd_ptr_##n[NUM_QUEUES(n)];                                     \
	static struct net_buf tx_frag_##n[NUM_QUEUES(n)][NUM_TX_DESC(n)];                          \
	static struct net_pkt tx_pkt_##n[NUM_QUEUES(n)][NUM_TX_DESC(n)];                           \
	static struct eth_intel_igc_intr_info intr_info_##n[NUM_MSIX(n)];                          \
	static msi_vector_t msi_vec_##n[NUM_MSIX(n)];

#define INTEL_IGC_MAC_DATA(n)                                                                      \
	static struct eth_intel_igc_mac_data eth_data_##n = {                                      \
		.tx =                                                                              \
			{                                                                          \
				.sem = tx_ring_lock_##n,                                           \
				.ring_wr_ptr = tx_ring_wr_ptr_##n,                                 \
				.ring_rd_ptr = tx_ring_rd_ptr_##n,                                 \
				.pkt = (struct net_pkt **)tx_pkt_##n,                              \
				.frag = (struct net_buf **)tx_frag_##n,                            \
			},                                                                         \
		.rx =                                                                              \
			{                                                                          \
				.sem = rx_ring_lock_##n,                                           \
				.ring_wr_ptr = rx_ring_wr_ptr_##n,                                 \
				.ring_rd_ptr = rx_ring_rd_ptr_##n,                                 \
			},                                                                         \
		.intr_info = intr_info_##n,                                                        \
		.msi_vec = msi_vec_##n,                                                            \
		.mac = DEVICE_DT_GET(DT_DRV_INST(n)),                                              \
		.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),                            \
	};

/**
 * @brief Initializes the configuration structure of each driver instance.
 */
#define INTEL_IGC_MAC_CONFIG(n)                                                                    \
	static void eth##n##_cfg_irq(const struct device *dev);                                    \
	static const struct eth_intel_igc_mac_cfg eth_cfg_##n = {                                  \
		.platform = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		.phy = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                              \
		.random_mac_address = DT_INST_PROP(n, zephyr_random_mac_address),                  \
		.config_func = eth##n##_cfg_irq,                                                   \
		.num_queues = NUM_QUEUES(n),                                                       \
		.num_msix = NUM_MSIX(n),                                                           \
		.num_tx_desc = NUM_TX_DESC(n),                                                     \
		.num_rx_desc = NUM_RX_DESC(n),                                                     \
	};

#define INTEL_IGC_MAC_INIT(n)                                                                      \
	DEVICE_PCIE_INST_DECLARE(n);                                                               \
	INTEL_IGC_MAC_CONFIG(n)                                                                    \
	INTEL_IGC_ALLOC_GLOBAL_OBJECTS(n)                                                          \
	INTEL_IGC_MAC_DATA(n)                                                                      \
	INTEL_IGC_SETUP_QUEUE_WORK(n);                                                             \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_intel_igc_init, NULL, &eth_data_##n, &eth_cfg_##n,    \
				      CONFIG_ETH_INIT_PRIORITY, &eth_api,                          \
				      CONFIG_ETH_INTEL_IGC_NET_MTU);                               \
	INTEL_IGC_MAC_CONFIG_IRQ(n)

DT_INST_FOREACH_STATUS_OKAY(INTEL_IGC_MAC_INIT)
