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

static uint32_t igc_read(mm_reg_t reg_addr, uint32_t offset)
{
	return sys_read32(reg_addr + offset);
}

static void igc_write(mm_reg_t reg_addr, uint32_t val)
{
	sys_write32(val, reg_addr);
}

/**
 * @brief Amend the register value as per the mask and set/clear the bit.
 */
static void igc_modify(mm_reg_t reg_addr, uint32_t offset, uint32_t mask, bool set)
{
	uint32_t val = igc_read(reg_addr, offset);

	if (set) {
		val |= mask;
	} else {
		val &= ~mask;
	}

	igc_write(reg_addr + offset, val);
}

/**
 * @brief Significant register changes required another register operation
 * to take effect. This status register read mimics that logic.
 */
static void igc_reg_refresh(mm_reg_t base)
{
	igc_read(base, INTEL_IGC_STATUS);
}

/**
 * @brief Get the index of a specific transmit descriptor within the ring.
 * This getter also works for multiple queues.
 */
static uint16_t get_tx_desc_idx(const struct device *dev,
				union dma_tx_desc *current_desc, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	union dma_tx_desc *tx_desc_base;

	tx_desc_base = (union dma_tx_desc *)&data->tx_desc[queue * cfg->num_tx_desc];

	return (current_desc - tx_desc_base);
}

/**
 * @brief Get the index of a specific receive descriptor within the ring.
 * This getter also works for multiple queues.
 */
static uint16_t get_rx_desc_idx(const struct device *dev,
				union dma_rx_desc *current_desc, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	union dma_rx_desc *rx_desc_base;

	rx_desc_base = (union dma_rx_desc *)&data->rx_desc[queue * cfg->num_rx_desc];

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
 * @brief This function retrieves the next available transmit descriptor from the ring
 * and ensures that the descriptor is available for DMA operation.
 */
static union dma_tx_desc *eth_intel_igc_get_txdesc(const struct device *dev, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t current_wr_idx, next_wr_idx;
	union dma_tx_desc *desc = NULL;

	k_sem_take(&data->tx_desc_ring_lock[queue], K_FOREVER);

	current_wr_idx = data->tx_desc_ring_wr_ptr[queue];
	next_wr_idx = (current_wr_idx + 1) % cfg->num_tx_desc;

	if (is_desc_unavailable(next_wr_idx, data->tx_desc_ring_rd_ptr[queue])) {
		k_sem_give(&data->tx_desc_ring_lock[queue]);
		return NULL;
	}
	desc = data->tx_desc + (queue * cfg->num_tx_desc + current_wr_idx);
	data->tx_desc_ring_wr_ptr[queue] = next_wr_idx;

	k_sem_give(&data->tx_desc_ring_lock[queue]);

	return desc;
}

/**
 * @brief This function checks if the DMA operation is complete by verifying the
 * writeback.dd bit. If the DMA operation is complete, it updates the read pointer
 * and clears the descriptor.
 */
static union dma_tx_desc *eth_intel_igc_release_txdesc(const struct device *dev,
						       uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t current_rd_idx, next_rd_idx;
	union dma_tx_desc *desc = NULL;

	k_sem_take(&data->tx_desc_ring_lock[queue], K_FOREVER);

	current_rd_idx = data->tx_desc_ring_rd_ptr[queue];
	desc = data->tx_desc + (queue * cfg->num_tx_desc + current_rd_idx);
	/* desc->writeback.dd set indicates that the DMA operation is complete */
	if (desc->writeback.dd) {
		next_rd_idx = (current_rd_idx + 1) % cfg->num_tx_desc;
		data->tx_desc_ring_rd_ptr[queue] = next_rd_idx;
		memset((void *)desc, 0, sizeof(union dma_tx_desc));
	} else {
		desc = NULL;
	}

	k_sem_give(&data->tx_desc_ring_lock[queue]);

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

	rd_idx = data->tx_desc_ring_rd_ptr[queue];
	while (rd_idx != data->tx_desc_ring_wr_ptr[queue]) {
		desc = (data->tx_desc + (queue * cfg->num_tx_desc + rd_idx));
		/* desc->writeback.dd not set indicates the DMA operation is not complete */
		if (!desc->writeback.dd) {
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
static union dma_rx_desc *eth_intel_igc_get_rxdesc(const struct device *dev,
						   uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t current_wr_idx, next_wr_idx;
	union dma_rx_desc *desc = NULL;

	k_sem_take(&data->rx_desc_ring_lock[queue], K_FOREVER);

	current_wr_idx  = data->rx_desc_ring_wr_ptr[queue];
	next_wr_idx = (current_wr_idx + 1) % cfg->num_rx_desc;

	if (is_desc_unavailable(next_wr_idx, data->rx_desc_ring_rd_ptr[queue])) {
		k_sem_give(&data->rx_desc_ring_lock[queue]);
		return NULL;
	}

	desc = data->rx_desc + (queue * cfg->num_rx_desc + current_wr_idx);
	data->rx_desc_ring_wr_ptr[queue] = next_wr_idx;

	k_sem_give(&data->rx_desc_ring_lock[queue]);

	return desc;
}

/**
 * @brief This function checks if the DMA operation is complete by verifying the
 * writeback.dd bit. If the DMA operation is complete, it updates the read pointer
 * and clears the descriptor.
 */
static union dma_rx_desc *eth_intel_igc_release_rxdesc(const struct device *dev,
						       uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t current_rd_idx, next_rd_idx;
	union dma_rx_desc *desc = NULL;

	k_sem_take(&data->rx_desc_ring_lock[queue], K_FOREVER);

	current_rd_idx = data->rx_desc_ring_rd_ptr[queue];
	desc = data->rx_desc + (queue * cfg->num_rx_desc + current_rd_idx);
	/* desc->writeback.dd set indicates that the DMA operation is complete */
	if (desc->writeback.dd) {
		next_rd_idx = (current_rd_idx + 1) % cfg->num_rx_desc;
		data->rx_desc_ring_rd_ptr[queue] = next_rd_idx;
	} else {
		desc = NULL;
	}

	k_sem_give(&data->rx_desc_ring_lock[queue]);

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

	idx = data->rx_desc_ring_rd_ptr[queue];
	while (idx != data->rx_desc_ring_wr_ptr[queue]) {
		desc = (data->rx_desc + (queue * cfg->num_rx_desc + idx));
		/* desc->writeback.dd not set indicates the DMA operation is not complete */
		if (!desc->writeback.dd) {
			break;
		}
		idx = (idx + 1) % cfg->num_rx_desc;
		count++;
	}

	return count;
}

/**
 * @brief Interrupt Service Routine for handling link status changes.
 */
static void eth_intel_igc_link_isr(void *arg)
{
	const struct device *dev = (struct device *)arg;
	struct eth_intel_igc_mac_data *data = dev->data;

	data->mac_dev = dev;

	k_work_submit(&data->link_work);
	igc_read(data->base, INTEL_IGC_ICR);

	/* Re-enable interrupt */
	igc_modify(data->base, INTEL_IGC_IMS, INTEL_IGC_LSC, true);
	igc_modify(data->base, INTEL_IGC_EIMS, BIT(data->link_vector_idx), true);
}

/**
 * @brief Interrupt Service Routine for handling queue interrupts.
 */
static void eth_intel_igc_queue_isr(void *arg)
{
	const struct eth_intel_igc_qvec *q_vector = (struct eth_intel_igc_qvec *)arg;
	const struct device *dev = q_vector->mac_dev;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint8_t queue = q_vector->vector_idx;

	igc_modify(data->base, INTEL_IGC_EICS, BIT(queue), false);
	k_work_submit(&data->tx_work[queue]);
	k_work_schedule(&data->rx_work[queue], K_MSEC(0));

	igc_read(data->base, INTEL_IGC_ICR);
	igc_modify(data->base, INTEL_IGC_EIMS, BIT(queue), true);
}

/**
 * @brief This function connects the MSI-X vectors to the respective interrupt handlers.
 */
static int eth_intel_igc_connect_all_msix_interrupts(const struct device *dev,
						     pcie_bdf_t bdf)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint8_t msix;

	/* Connect the MSI-X vector to the Queue interrupt handler */
	for (msix = 0; msix < cfg->num_queues; msix++) {
		struct eth_intel_igc_qvec *qvec = &data->q_vector[msix];

		/* These references will be used during isr */
		qvec->vector_idx = msix;
		qvec->mac_dev = dev;
		if (!pcie_msi_vector_connect(bdf, &data->vectors[msix],
					     (void *)eth_intel_igc_queue_isr,
					     qvec, 0)) {
			LOG_ERR("Failed to connect queue_%d interrupt handler", msix);
			return -EIO;
		}
	}

	/* Connect the last MSI-X vector to the Link interrupt handler */
	data->link_vector_idx = msix;
	if (!pcie_msi_vector_connect(bdf, &data->vectors[data->link_vector_idx],
				     (void *)eth_intel_igc_link_isr, dev, 0)) {
		LOG_ERR("Failed to connect link interrupt handler");
		return -EIO;
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

	ret = pcie_msi_vectors_allocate(bdf, CONFIG_ETH_INTEL_IGC_INT_PRIORITY,
					data->vectors, cfg->num_msix);
	if (ret < cfg->num_msix) {
		LOG_ERR("Failed to allocate MSI-X vectors");
		return -EIO;
	}

	ret = eth_intel_igc_connect_all_msix_interrupts(dev, bdf);
	if (ret < 0) {
		return ret;
	}

	if (!pcie_msi_enable(bdf, data->vectors, cfg->num_msix, 0)) {
		LOG_ERR("Failed to enable MSI-X vectors");
		return -EIO;
	}

	return 0;
}

/**
 * @brief This function maps the IGC device interrupts to the MSI-X vectors using IVAR.
 */
static void eth_intel_igc_map_interrupt_to_vector(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t msix_cfg, msix, reg_idx;

	/* Set MSI-X capability */
	igc_modify(data->base, INTEL_IGC_GPIE,
		   INTEL_IGC_GPIE_NSICR | INTEL_IGC_GPIE_MSIX_MODE |
		   INTEL_IGC_GPIE_EIAME | INTEL_IGC_GPIE_PBA, true);

	/* Configure IVAR RX and TX for each queue interrupt */
	for (msix = 0; msix < cfg->num_queues; msix++) {
		reg_idx = msix >> 1;
		msix_cfg = igc_read(data->base,
				    (INTEL_IGC_IVAR_BASE_ADDR + (reg_idx << 2)));
		if (msix % 2) {
			msix_cfg &= INTEL_IGC_IVAR_MSI_CLEAR_RX1_RX3;
			msix_cfg |= (msix | INTEL_IGC_IVAR_INT_VALID_BIT) << 16;
			msix_cfg &= INTEL_IGC_IVAR_MSI_CLEAR_TX1_TX3;
			msix_cfg |= (msix | INTEL_IGC_IVAR_INT_VALID_BIT) << 24;
		} else {
			msix_cfg &= INTEL_IGC_IVAR_MSI_CLEAR_RX0_RX2;
			msix_cfg |= (msix | INTEL_IGC_IVAR_INT_VALID_BIT) << 0;
			msix_cfg &= INTEL_IGC_IVAR_MSI_CLEAR_TX0_TX2;
			msix_cfg |= (msix | INTEL_IGC_IVAR_INT_VALID_BIT) << 8;
		}
		igc_write(data->base +
			  (INTEL_IGC_IVAR_BASE_ADDR + (reg_idx << 2)), msix_cfg);
	}

	/* Configure IVAR MISC interrupt */
	msix_cfg = (msix | INTEL_IGC_IVAR_INT_VALID_BIT) << 8;
	igc_modify(data->base, INTEL_IGC_IVAR_MISC, msix_cfg, true);
}

/**
 * @brief Helper function to write MAC address to RAL and RAH registers.
 */
static void eth_intel_igc_set_mac_addr(mm_reg_t reg_addr, int index,
				       const uint8_t *addr, uint32_t rah_flags)
{
	uint32_t rah = rah_flags | (addr[5] << 8) | addr[4];
	uint32_t ral = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];

	igc_write(reg_addr + INTEL_IGC_RAH(index), rah);
	igc_reg_refresh(reg_addr);
	igc_write(reg_addr + INTEL_IGC_RAL(index), ral);
	igc_reg_refresh(reg_addr);
}

/**
 * @brief This function configures the MAC address filtering for the IGC, allowing it
 * to filter packets based on the MAC address and filter mode.
 */
static void eth_intel_igc_set_mac_filter(const struct device *dev,
					 enum eth_igc_mac_filter_mode mode,
					 const uint8_t *addr,
					 int index,
					 uint8_t queue)
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
	eth_intel_igc_set_mac_addr(data->base, index, addr, config);
}

static void eth_intel_igc_get_phy_link_state_speed(struct eth_intel_igc_mac_data *data)
{
	uint32_t reg = igc_read(data->base, INTEL_IGC_STATUS);

	if (reg & INTEL_IGC_STATUS_LU) {
		net_eth_carrier_on(data->iface);
		LOG_INF("%s Duplex Link Up & Speed %s",
			(reg & INTEL_IGC_STATUS_FD) ? "Full" : "Half",
			(reg & INTEL_IGC_STATUS_SPEED_1000) ? "1000Mbps" :
			((reg & INTEL_IGC_STATUS_SPEED_100) ? "100Mbps"  : "10Mbps"));
	} else {
		if (net_if_is_carrier_ok(data->iface)) {
			net_eth_carrier_off(data->iface);
		}
		LOG_ERR("Link Down");
	}
}

static void eth_intel_igc_get_link_status(struct k_work *work)
{
	struct eth_intel_igc_mac_data *data = CONTAINER_OF(work,
							   struct eth_intel_igc_mac_data,
							   link_work);

	eth_intel_igc_get_phy_link_state_speed(data);
}

static void eth_intel_igc_interrupt_enable(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t config = 0;

	/* Clear pending interrupt */
	igc_read(data->base, INTEL_IGC_ICR);
	igc_write(data->base + INTEL_IGC_ICS, INTEL_IGC_LSC);

	/* Prepare bit mask of queue and link interrupt */
	config  = BIT_MASK(cfg->num_queues);
	config |= BIT(data->link_vector_idx);
	igc_write(data->base + INTEL_IGC_EIAC, config);
	igc_modify(data->base, INTEL_IGC_EIAM, config, true);
	igc_write(data->base + INTEL_IGC_EIMS, config);
	igc_modify(data->base, INTEL_IGC_IMS, INTEL_IGC_LSC, true);
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
	if (net_if_set_link_addr(data->iface, data->mac_addr.octets,
	    sizeof(data->mac_addr.octets), NET_LINK_ETHERNET) < 0) {
		LOG_ERR("Failed to set mac address");
		return;
	}

	/* Configure interrupt vectors and handlers */
	cfg->config_func(dev);

	eth_intel_igc_interrupt_enable(dev);
}

static enum ethernet_hw_caps eth_intel_igc_get_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (ETHERNET_LINK_10BASE_T   |
		ETHERNET_LINK_100BASE_T  |
		ETHERNET_LINK_1000BASE_T |
		ETHERNET_DUPLEX_SET);
}

static int eth_intel_igc_set_config(const struct device *dev,
				    enum ethernet_config_type type,
				    const struct ethernet_config *eth_cfg)
{
	struct eth_intel_igc_mac_data *data = dev->data;
	const struct ethphy_driver_api *phy_api = data->phy->api;
	const struct device *phydev = data->phy;
	int ret = 0;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr.octets, eth_cfg->mac_address.addr,
		       sizeof(eth_cfg->mac_address));
		net_if_set_link_addr(data->iface, data->mac_addr.octets,
				     sizeof(data->mac_addr.octets),
				     NET_LINK_ETHERNET);

		eth_intel_igc_set_mac_filter(dev, DEST_ADDR, data->mac_addr.octets, 0, 0);
		break;
	case ETHERNET_CONFIG_TYPE_LINK:
		enum phy_link_speed adv_speeds = 0;

		if (eth_cfg->l.link_1000bt) {
			adv_speeds = LINK_FULL_1000BASE_T;
		} else if (eth_cfg->l.link_100bt) {
			adv_speeds = LINK_FULL_100BASE_T;
		} else if (eth_cfg->l.link_10bt) {
			adv_speeds = LINK_FULL_10BASE_T;
		} else {
			LOG_ERR("Link Speed Not Supported");
			ret = -ENOTSUP;
			break;
		}
		ret = phy_api->cfg_link(phydev, adv_speeds);
		if (ret < 0) {
			LOG_ERR("Failed to set link speed");
			ret = -EIO;
		}
		break;
	default:
		LOG_ERR("Link Speed Not Supported");
		ret = -ENOTSUP;
		break;
	};

	return ret;
}

#ifdef CONFIG_NET_STATISTICS_ETHERNET
static struct net_stats_eth *eth_intel_igc_get_stats(const struct device *dev)
{
	struct eth_intel_igc_mac_data *data = dev->data;
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct net_stats_eth *stats = &data->stats;

	stats->bytes.sent += igc_read(data->base, INTEL_IGC_TOTL);
	stats->bytes.received += igc_read(data->base, INTEL_IGC_TORL);
	stats->pkts.tx += igc_read(data->base, INTEL_IGC_TPT);
	stats->pkts.rx += igc_read(data->base, INTEL_IGC_TPR);
	stats->broadcast.tx += igc_read(data->base, INTEL_IGC_BPTC);
	stats->broadcast.rx += igc_read(data->base, INTEL_IGC_BPRC);
	stats->multicast.tx += igc_read(data->base, INTEL_IGC_MPTC);
	stats->multicast.rx += igc_read(data->base, INTEL_IGC_MPRC);
	stats->errors.rx += igc_read(data->base, INTEL_IGC_RERC);
	stats->error_details.rx_length_errors += igc_read(data->base, INTEL_IGC_RLEC);
	stats->error_details.rx_crc_errors += igc_read(data->base, INTEL_IGC_CRCERRS);
	stats->error_details.rx_frame_errors += igc_read(data->base, INTEL_IGC_RJC);
	stats->error_details.rx_no_buffer_count += igc_read(data->base, INTEL_IGC_RNBC);
	stats->error_details.rx_long_length_errors += igc_read(data->base, INTEL_IGC_ROC);
	stats->error_details.rx_short_length_errors += igc_read(data->base, INTEL_IGC_RUC);
	stats->error_details.rx_align_errors += igc_read(data->base, INTEL_IGC_ALGNERRC);
	stats->error_details.rx_buf_alloc_failed += igc_read(data->base, INTEL_IGC_MPC);
	stats->error_details.tx_aborted_errors += igc_read(data->base, INTEL_IGC_DC);
	stats->flow_control.rx_flow_control_xon += igc_read(data->base, INTEL_IGC_XONRXC);
	stats->flow_control.rx_flow_control_xoff += igc_read(data->base, INTEL_IGC_XOFFRXC);
	stats->flow_control.tx_flow_control_xon += igc_read(data->base, INTEL_IGC_XONTXC);
	stats->flow_control.tx_flow_control_xoff += igc_read(data->base, INTEL_IGC_XOFFTXC);
	stats->collisions += igc_read(data->base, INTEL_IGC_COLC);
	for (int q = 0; q < cfg->num_queues; q++) {
		stats->tx_dropped += igc_read(data->base, INTEL_IGC_TQDPC(q));
	}

	return &data->stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

/**
 * @brief This function clean and unref the net_buf of the given fragment. It ensures
 * to avoid potential memory leaks.
 */
static void eth_intel_igc_frag_clean_unref(struct net_buf *frag)
{
	if (frag) {
		if (frag->data) {
			memset((void *)frag->data, 0, frag->len);
		}
		net_pkt_frag_unref(frag);
	}
}

/**
 * @brief This function clean and unref the net_pkt of the given packet. It ensures
 * to avoid potential memory leaks.
 */
static void eth_intel_igc_pkt_clean_unref(struct net_pkt *pkt)
{
	if (pkt) {
		if (pkt->frags->data) {
			memset((void *)pkt->frags->data, 0, pkt->frags->len);
		}
		net_pkt_unref(pkt);
	}
}

/**
 * @brief This function releases completed transmit descriptors, cleans up the associated
 * net_buf and net_pkt, and frees any allocated resources.
 */
static void eth_intel_igc_tx_clean(struct eth_intel_igc_mac_data *data, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = data->mac_dev->config;
	uint32_t desc_idx = 0, tx_clean_count;
	union dma_tx_desc *desc;
	struct net_buf *frag;
	struct net_pkt *pkt;

	tx_clean_count = eth_intel_igc_completed_txdesc_num(data->mac_dev, queue);
	for (uint8_t count = 0; count < tx_clean_count; count++) {
		desc = eth_intel_igc_release_txdesc(data->mac_dev, queue);
		if (!desc) {
			LOG_ERR("No more tx descriptor available to release");
			continue;
		}

		desc_idx = get_tx_desc_idx(data->mac_dev, desc, queue);
		frag = *(data->tx_frag + queue * cfg->num_tx_desc + desc_idx);
		pkt  = *(data->tx_pkt  + queue * cfg->num_tx_desc + desc_idx);
		if (pkt) {
			eth_intel_igc_frag_clean_unref(frag);
			eth_intel_igc_pkt_clean_unref(pkt);
		}
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
	uint32_t desc_idx = 0;

	desc = eth_intel_igc_get_txdesc(data->mac_dev, queue);
	if (!desc) {
		LOG_ERR("No more tx desc available");
		return -ENOMEM;
	}

	desc_idx = get_tx_desc_idx(data->mac_dev, desc, queue);
	/* Store the pkt and first frag, then clear it during transmit clean. */
	*(data->tx_frag + queue * cfg->num_tx_desc + desc_idx) = frag->frags ? 0 : pkt->frags;
	*(data->tx_pkt + queue * cfg->num_tx_desc + desc_idx) = frag->frags ? 0 : pkt;

	desc->read.data_buf_addr = (uint64_t)sys_cpu_to_le64((uint64_t)frag->data);
	/* Copy the total payload length */
	desc->read.payloadlen = total_len;
	/* Copy the particular frag's buffer length */
	desc->read.data_len = frag->len;
	desc->read.desc_type = INTEL_IGC_TX_DESC_TYPE;
	desc->read.ifcs = true;
	desc->read.dext = true;

	if (!frag->frags) {
		desc->read.eop = true;
		desc->read.rs = true;

		igc_write((data->base + INTEL_IGC_TDT(queue)), data->tx_desc_ring_wr_ptr[queue]);
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
	struct net_buf *frag;
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

	for (frag = pkt->frags; frag; frag = frag->frags) {
		/* Hold header fragment until transmit clean done */
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
static sa_family_t eth_intel_igc_get_sa_family(volatile uint8_t *rx_buf)
{
	struct net_eth_hdr *eth_hdr = (struct net_eth_hdr *)rx_buf;
	sa_family_t family;

	switch (ntohs(eth_hdr->type)) {
	case NET_ETH_PTYPE_IP:
		family = AF_INET;
		break;
	case NET_ETH_PTYPE_IPV6:
		family = AF_INET6;
		break;
	default:
		family = AF_UNSPEC;
		break;
	}

	return family;
}

/**
 * @brief This function updates the tail pointer of the RX descriptor ring, retrieves the
 * next available RX descriptor, and prepare it for receiving incoming packets by setting
 * the packet buffer address.
 */
static void eth_intel_igc_rx_data_hdl(struct eth_intel_igc_mac_data *data, uint8_t queue,
				      uint32_t idx, union dma_rx_desc *desc)
{
	const struct eth_intel_igc_mac_cfg *cfg = data->mac_dev->config;

	igc_write((data->base + INTEL_IGC_RDT(queue)), idx);
	desc = eth_intel_igc_get_rxdesc(data->mac_dev, queue);
	if (!desc) {
		LOG_ERR("No more rx descriptor available");
		return;
	}

	idx = get_rx_desc_idx(data->mac_dev, desc, queue);
	desc->read.pkt_buf_addr = (uint64_t)sys_cpu_to_le64((uint64_t)(data->rx_buf +
				  (queue * cfg->num_rx_desc + idx) * ETH_MAX_FRAME_SZ));
	desc->writeback.dd = 0;
}

static void eth_intel_igc_rx_data_hdl_err(struct eth_intel_igc_mac_data *data, uint8_t queue,
					  uint32_t idx, union dma_rx_desc *desc,
					  struct net_pkt *pkt)
{
	eth_intel_igc_pkt_clean_unref(pkt);
	eth_intel_igc_rx_data_hdl(data, queue, idx, desc);
}

/**
 * @brief This function retrieves completed receive descriptors, allocates net_pkt
 * buffers, copies the received data into the buffers, and submits the packets to
 * the network stack.
 */
static void eth_intel_igc_rx_data(struct eth_intel_igc_mac_data *data, uint8_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = data->mac_dev->config;
	uint32_t count, idx = 0, completed_count = 0;
	volatile uint8_t *rx_buf;
	union dma_rx_desc *desc;
	struct net_pkt *pkt;
	int ret;

	completed_count = eth_intel_igc_completed_rxdesc_num(data->mac_dev, queue);
	for (count = 0; count < completed_count; count++) {
		/* Retrieve the position of next descriptor to be processed */
		idx = data->rx_desc_ring_rd_ptr[queue];
		desc = eth_intel_igc_release_rxdesc(data->mac_dev, queue);
		if (!desc) {
			LOG_ERR("RX descriptor is NULL");
			continue;
		}

		if (!net_if_is_up(data->iface) || !desc->writeback.pkt_len) {
			LOG_ERR("RX interface is down or pkt_len is %d",
				desc->writeback.pkt_len);
			eth_intel_igc_rx_data_hdl_err(data, queue, idx, desc, NULL);
			continue;
		}

		/* Get the DMA buffer pointer as per index */
		rx_buf = data->rx_buf + ((queue * cfg->num_rx_desc + idx) *
					 ETH_MAX_FRAME_SZ);

		/* Allocate packet buffer as per address family */
		pkt = net_pkt_rx_alloc_with_buffer(data->iface, desc->writeback.pkt_len,
						   eth_intel_igc_get_sa_family(rx_buf),
						   0, K_MSEC(200));
		if (!pkt) {
			LOG_ERR("Failed to allocate RX buffer");
			eth_intel_igc_rx_data_hdl_err(data, queue, idx, desc, NULL);
			continue;
		}

		/* Write DMA buffer to packet */
		ret = net_pkt_write(pkt, (void *)rx_buf, desc->writeback.pkt_len);
		if (ret < 0) {
			LOG_ERR("Failed to write RX buffer to pkt");
			eth_intel_igc_rx_data_hdl_err(data, queue, idx, desc, pkt);
			continue;
		}

		/* Process received packet */
		ret = net_recv_data(data->iface, pkt);
		if (ret < 0) {
			LOG_ERR("Failed to enqueue the RX pkt: %d", queue);
			eth_intel_igc_rx_data_hdl_err(data, queue, idx, desc, pkt);
			continue;
		}

		eth_intel_igc_rx_data_hdl(data, queue, idx, desc);
	}
}

/**
 * @brief Configure and Enable the Transmit Control Register.
 */
static void eth_intel_igc_tctl_setup(mm_reg_t reg_addr)
{
	uint32_t config = igc_read(reg_addr, 0);

	config &= ~INTEL_IGC_TCTL_CT;
	config |= INTEL_IGC_TCTL_EN   |
		  INTEL_IGC_TCTL_PSP  |
		  INTEL_IGC_TCTL_RTLC |
		  INTEL_IGC_COLLISION_THRESHOLD << INTEL_IGC_TCTL_CT_SHIFT;

	igc_write(reg_addr, config);
}

/**
 * @brief IGC expects the DMA descriptors to be aligned to 128-byte boundaries for efficient
 * access and to avoid potential data corruption or performance issues.
 */
static void *eth_intel_igc_aligned_alloc(uint32_t size)
{
	void *desc_base = k_aligned_alloc(4096, size);

	if (!desc_base) {
		LOG_ERR("Failed to allocate aligned descriptor base");
		return NULL;
	}
	memset(desc_base, 0, size);

	return desc_base;
}

/**
 * @brief This function initializes the transmit descriptor ring for the specified queue.
 */
static void eth_intel_igc_init_tx_desc_ring(const struct device *dev, uint32_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint64_t desc_addr;
	uint32_t config;

	/* Disable the transmit descriptor ring */
	igc_write(data->base + INTEL_IGC_TXDCTL(queue), 0);
	igc_reg_refresh(data->base);

	/* Program the transmit descriptor ring total length */
	igc_write(data->base + INTEL_IGC_TDLEN(queue),
		  cfg->num_tx_desc * sizeof(union dma_tx_desc));

	/* Program the descriptor base address */
	desc_addr = (uint64_t)(data->tx_desc + (queue * cfg->num_tx_desc));
	igc_write(data->base + INTEL_IGC_TDBAH(queue), (uint32_t)(desc_addr >> 32));
	igc_write(data->base + INTEL_IGC_TDBAL(queue), (uint32_t)(desc_addr >>  0));

	/* Reset Head and Tail Descriptor Pointers */
	igc_write(data->base + INTEL_IGC_TDH(queue), 0);
	igc_write(data->base + INTEL_IGC_TDT(queue), 0);

	/* Configure TX DMA interrupt trigger thresholds */
	config = INTEL_IGC_TX_PTHRESH_VAL << INTEL_IGC_TX_PTHRESH_SHIFT |
		 INTEL_IGC_TX_HTHRESH_VAL << INTEL_IGC_TX_HTHRESH_SHIFT |
		 INTEL_IGC_TX_WTHRESH_VAL << INTEL_IGC_TX_WTHRESH_SHIFT;
	/* Enable the transmit descriptor ring */
	config |= INTEL_IGC_TXDCTL_QUEUE_ENABLE;
	igc_write(data->base + INTEL_IGC_TXDCTL(queue), config);
}

/**
 * @brief This function allocates and initializes the transmit descriptor rings for each
 * queue. It sets up the descriptor base addresses, lengths, and control registers.
 */
static int eth_intel_igc_tx_dma_init(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t queue, size;

	/* Calculate the total size of the TX descriptor buffer */
	size = cfg->num_queues * cfg->num_tx_desc * sizeof(union dma_tx_desc);
	/* Allocate memory for the TX descriptor buffer */
	size = ROUND_UP(size, sizeof(union dma_tx_desc));
	data->tx_desc = (union dma_tx_desc *)eth_intel_igc_aligned_alloc(size);
	if (data->tx_desc == NULL) {
		LOG_ERR("TX descriptor buf alloc failed");
		return -ENOBUFS;
	}

	for (queue = 0; queue < cfg->num_queues; queue++) {
		k_sem_init(&data->tx_desc_ring_lock[queue], 1, 1);
		eth_intel_igc_init_tx_desc_ring(dev, queue);
	}

	/* Configure internal transmit descriptor buffer size */
	igc_write(data->base + INTEL_IGC_TXPBS, INTEL_IGC_TXPBS_TXPBSIZE_DEFAULT);
	eth_intel_igc_tctl_setup(data->base + INTEL_IGC_TCTL);

	return 0;
}

/**
 * @brief Configure and Enable the Receive Control Register.
 */
static void eth_intel_igc_rctl_setup(mm_reg_t reg_addr)
{
	uint32_t config = igc_read(reg_addr, 0);

	/* Multicast / Unicast Table Offset */
	config &= ~(0x3 << INTEL_IGC_RCTL_MO_SHIFT);
	/* Do not store bad packets */
	config &= ~INTEL_IGC_RCTL_SBP;
	/* Turn off VLAN filters */
	config &= ~INTEL_IGC_RCTL_VFE;
	config |= INTEL_IGC_RCTL_EN
		  | INTEL_IGC_RCTL_BAM
	/* Enable long packet receive */
		  | INTEL_IGC_RCTL_LPE
	/* Strip the CRC */
		  | INTEL_IGC_RCTL_SECRC
		  | INTEL_IGC_RCTL_SZ_2048;

	igc_write(reg_addr, config);
}

/**
 * @brief This function ensures that each Receive DMA descriptor is properly prepared
 * with a pre-allocated packet buffer, enabling the device to receive and store incoming
 * packets efficiently.
 */
static int eth_intel_igc_rx_desc_prepare(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	union dma_rx_desc *desc;
	uint32_t queue, desc_idx;
	volatile uint8_t *buf;

	/* Allocate memory for rx DMA buffer */
	data->rx_buf = (uint8_t *)k_calloc(cfg->num_queues * cfg->num_rx_desc, ETH_MAX_FRAME_SZ);
	if (!data->rx_buf) {
		LOG_ERR("rx buffer alloc failed");
		return -ENOBUFS;
	}

	/* Assign allocated memory to each descriptor as per it's index */
	for (queue = 0; queue < cfg->num_queues; queue++) {
		desc = &data->rx_desc[queue * cfg->num_rx_desc];

		for (desc_idx = 0; desc_idx < cfg->num_rx_desc; desc_idx++) {
			/* Set the pkt buffer address */
			buf = data->rx_buf + ((queue * cfg->num_rx_desc + desc_idx) *
					       ETH_MAX_FRAME_SZ);
			desc[desc_idx].read.pkt_buf_addr =
				(uint64_t)sys_cpu_to_le64((uint64_t)buf);
			desc[desc_idx].read.hdr_buf_addr = (uint64_t)&desc[desc_idx];
		}

		/* Update tail pointer in hardware and copy the same for driver reference */
		igc_write(data->base + INTEL_IGC_RDT(queue), cfg->num_rx_desc - 1);
		data->rx_desc_ring_wr_ptr[queue] = cfg->num_rx_desc - 1;
	}

	return 0;
}

/**
 * @brief This function initializes the receive descriptor ring for the given queue.
 */
static void eth_intel_igc_init_rx_desc_ring(const struct device *dev, uint32_t queue)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint64_t desc_addr;
	uint32_t config;

	/* Disable the receive descriptor ring */
	igc_write(data->base + INTEL_IGC_RXDCTL(queue), 0);
	igc_reg_refresh(data->base);

	/* Program the receive descriptor ring total length */
	igc_write(data->base + INTEL_IGC_RDLEN(queue),
		  cfg->num_rx_desc * sizeof(union dma_rx_desc));

	/* Program the descriptor base address */
	desc_addr = (uint64_t)(data->rx_desc + (queue * cfg->num_rx_desc));
	igc_write(data->base + INTEL_IGC_RDBAH(queue), (uint32_t)(desc_addr >> 32));
	igc_write(data->base + INTEL_IGC_RDBAL(queue), (uint32_t)(desc_addr >>  0));

	/* Configure the receive descriptor control */
	config  = INTEL_IGC_SRRCTL_BSIZEPKT(ETH_MAX_FRAME_SZ);
	config |= INTEL_IGC_SRRCTL_BSIZEHDR(INTEL_IGC_RXBUFFER_256);
	config |= INTEL_IGC_SRRCTL_DESCTYPE_ADV_ONEBUF;
	config |= INTEL_IGC_SRRCTL_DROP_EN;
	igc_write(data->base + INTEL_IGC_SRRCTL(queue), config);

	/* Reset Head and Tail Descriptor Pointers */
	igc_write(data->base + INTEL_IGC_RDH(queue), 0);
	igc_write(data->base + INTEL_IGC_RDT(queue), 0);

	config  = sys_read32(data->base + INTEL_IGC_RXDCTL(queue));
	config &= INTEL_IGC_RX_THRESH_RESET;
	/* Configure RX DMA interrupt trigger thresholds */
	config |= INTEL_IGC_RX_PTHRESH_VAL << INTEL_IGC_RX_PTHRESH_SHIFT |
		  INTEL_IGC_RX_HTHRESH_VAL << INTEL_IGC_RX_HTHRESH_SHIFT |
		  INTEL_IGC_RX_WTHRESH_VAL << INTEL_IGC_RX_WTHRESH_SHIFT;
	/* Enable the receive descriptor ring */
	config |= INTEL_IGC_RXDCTL_QUEUE_ENABLE;
	igc_write(data->base + INTEL_IGC_RXDCTL(queue), config);
	igc_reg_refresh(data->base);
}

/**
 * @brief This function initializes the receive descriptor ring for the given queue.
 * It sets up the descriptor base address, length, and control registers.
 */
static int eth_intel_igc_rx_dma_init(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	uint32_t queue, size;
	int ret;

	/*
	 * TODO: enable RSS mapping.
	 * TODO: enable interrupt moderation here.
	 * TODO: enable Checksum offload here.
	 * TODO: enable VLAN offload here.
	 */

	/* Disable receive logic until descriptor setup */
	igc_modify(data->base, INTEL_IGC_RCTL, INTEL_IGC_RCTL_EN, false);
	igc_write(data->base + INTEL_IGC_RXCSUM, 0);

	/* Calculate the total size of the RX descriptor buffer */
	size = cfg->num_queues * cfg->num_rx_desc * sizeof(union dma_rx_desc);
	size = ROUND_UP(size, sizeof(union dma_rx_desc));

	/* Allocate memory for the RX descriptor buffer */
	data->rx_desc = (union dma_rx_desc *)eth_intel_igc_aligned_alloc(size);
	if (!data->rx_desc) {
		LOG_ERR("RX descriptor buf alloc failed");
		return -ENOBUFS;
	}

	for (queue = 0; queue < cfg->num_queues; queue++) {
		k_sem_init(&data->rx_desc_ring_lock[queue], 1, 1);
		eth_intel_igc_init_rx_desc_ring(dev, queue);
	}

	/* Configure internal receive descriptor buffer size */
	igc_write(data->base + INTEL_IGC_RXPBS, INTEL_IGC_RXPBS_RXPBSIZE_DEFAULT);

	ret = eth_intel_igc_rx_desc_prepare(dev);
	if (ret < 0) {
		LOG_ERR("Rx descriptor prepare failed");
		return ret;
	}
	eth_intel_igc_rctl_setup(data->base + INTEL_IGC_RCTL);

	return ret;
}

/**
 * @brief This function validates the MAC address and returns true on success.
 */
static bool eth_intel_igc_is_valid_mac_addr(uint8_t *addr)
{
	if (UNALIGNED_GET((uint32_t *)addr) == INTEL_IGC_DEF_MAC_ADDR) {
		LOG_WRN("Wrong MAC address addr=%02X:%02X:%02X:%02X:%02X:%02X",
			addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
		return false;
	}

	if (addr[0] & 0x01) {
		LOG_WRN("Multicast MAC address");
		return false;
	}

	if ((UNALIGNED_GET((uint32_t *)addr) == 0x00000000) &&
	    (UNALIGNED_GET((uint16_t *)(addr + 4)) == 0x0000)) {
		LOG_WRN("Invalid Mac Address");
		return false;
	}

	return true;
}

static void eth_intel_igc_get_nvm_mac_addr(struct eth_intel_igc_mac_data *data,
					   union eth_intel_igc_mac_addr *mac_addr)
{
	/* Reading RAL/RAH will require at least 2us upon MAC reset */
	WAIT_FOR(sys_read32(data->base + INTEL_IGC_RAL(0)),
		INTEL_IGC_READ_NVM_TIMEOUT, k_usleep(10));

	mac_addr->addr_lo = igc_read(data->base, INTEL_IGC_RAL(0));
	mac_addr->addr_hi = igc_read(data->base, INTEL_IGC_RAH(0));
}

static void eth_intel_igc_get_mac_addr(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;

	if (cfg->mac_addr_exist_in_nvm) {
		LOG_INF("Read MAC address from EEPROM");
		eth_intel_igc_get_nvm_mac_addr(data, &data->mac_addr);
	}

	if (!cfg->mac_addr_exist_in_nvm ||
	    !eth_intel_igc_is_valid_mac_addr(data->mac_addr.octets)) {
		LOG_DBG("Read MAC address from Device Tree");
		cfg->assign_mac_addr(data->mac_addr.octets);
		if (!eth_intel_igc_is_valid_mac_addr(data->mac_addr.octets)) {
			LOG_WRN("Invalid MAC address. Use Random Mac Address");
			gen_random_mac(data->mac_addr.octets, 0, 0xA0, 0xC9);
		}
	}
}

static void eth_intel_igc_rx_addrs_init(const struct device *dev)
{
	struct eth_intel_igc_mac_data *data = dev->data;
	uint8_t def_addr[INTEL_IGC_ALEN] = {0};
	uint16_t rar_count = 128;

	eth_intel_igc_get_mac_addr(dev);
	LOG_INF("IGC MAC addr %02X:%02X:%02X:%02X:%02X:%02X",
		data->mac_addr.octets[0], data->mac_addr.octets[1],
		data->mac_addr.octets[2], data->mac_addr.octets[3],
		data->mac_addr.octets[4], data->mac_addr.octets[5]);

	/* Set default MAC address */
	eth_intel_igc_set_mac_addr(data->base, 0, data->mac_addr.octets, INTEL_IGC_RAH_AV);

	/* Program default receive address register as zero */
	for (uint8_t rar = 1; rar < rar_count; rar++) {
		eth_intel_igc_set_mac_addr(data->base, rar, def_addr, INTEL_IGC_RAH_AV);
	}
}

static int eth_intel_igc_validate_sku(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	pcie_id_t pcie_id;

	pcie_id = eth_intel_get_pcie_id(cfg->platform);
	switch (PCIE_ID_TO_DEV(pcie_id)) {
	case INTEL_IGC_I226_LMVP:
	case INTEL_IGC_I226_LM:
	case INTEL_IGC_I226_V:
	case INTEL_IGC_I226_IT:
		return 0;
	case INTEL_IGC_I226_BLANK_NVM:
	default:
		break;
	}
	LOG_ERR("IGC SKU validation failed pcie_id - 0x%x", pcie_id);

	return -EIO;
}

/**
 * @brief This function disables the PCIe master access to the device, ensuring that
 * the device is ready to be controlled by the driver.
 */
static int eth_intel_igc_disable_pcie_master(struct eth_intel_igc_mac_data *data)
{
	uint32_t timeout = INTEL_IGC_GIO_MASTER_DISABLE_TIMEOUT;

	igc_modify(data->base, INTEL_IGC_CTRL, INTEL_IGC_CTRL_GIO_MASTER_DISABLE, true);

	/* Wait for the INTEL_IGC_STATUS_GIO_MASTER_ENABLE bit to clear */
	if (!WAIT_FOR(!(igc_read(data->base, INTEL_IGC_STATUS) &
		     INTEL_IGC_STATUS_GIO_MASTER_ENABLE), timeout, k_msleep(1))) {
		LOG_ERR("Pending for GIO Master Request");
		return -EIO;
	}

	return 0;
}

static void eth_intel_igc_init_link_work(const struct device *dev)
{
	struct eth_intel_igc_mac_data *data = dev->data;

	/* Duplex and Speed settings of MAC will resolve automatically based on PHY */
	igc_modify(data->base, INTEL_IGC_CTRL,
		   INTEL_IGC_CTRL_FRCSPD | INTEL_IGC_CTRL_FRCDPX, false);
	igc_modify(data->base, INTEL_IGC_CTRL, INTEL_IGC_CTRL_SLU, true);

	k_work_init(&data->link_work, eth_intel_igc_get_link_status);
}

static void eth_intel_igc_get_dev_ownership(struct eth_intel_igc_mac_data *data)
{
	igc_modify(data->base, INTEL_IGC_CTRL_EXT, INTEL_IGC_CTRL_EXT_DRV_LOAD, true);
}

static int eth_intel_igc_init_mac_hw(const struct device *dev)
{
	struct eth_intel_igc_mac_data *data = dev->data;
	int ret = 0;

	ret = eth_intel_igc_disable_pcie_master(data);
	if (ret < 0) {
		LOG_ERR("Disable pcie master failed");
		return ret;
	}

	igc_write((data->base + INTEL_IGC_IMC), UINT32_MAX);
	igc_write((data->base + INTEL_IGC_RCTL), 0);
	igc_write((data->base + INTEL_IGC_TCTL), INTEL_IGC_TCTL_PSP);
	igc_reg_refresh(data->base);
	k_msleep(20);

	/* MAC Reset */
	igc_modify(data->base, INTEL_IGC_CTRL, INTEL_IGC_CTRL_DEV_RST, true);

	/* Setup receive address */
	eth_intel_igc_rx_addrs_init(dev);

	eth_intel_igc_get_dev_ownership(data);

	return ret;
}

static int eth_intel_igc_init(const struct device *dev)
{
	const struct eth_intel_igc_mac_cfg *cfg = dev->config;
	struct eth_intel_igc_mac_data *data = dev->data;
	int ret = 0;

	data->base = DEVICE_MMIO_GET(cfg->platform);
	if (!data->base) {
		LOG_ERR("Failed to get MMIO base address");
		return -ENODEV;
	}

	ret = eth_intel_igc_validate_sku(dev);
	if (ret) {
		return ret;
	}

	ret = eth_intel_igc_init_mac_hw(dev);
	if (ret) {
		return ret;
	}

	ret = eth_intel_igc_pcie_msix_setup(dev);
	if (ret) {
		return ret;
	}

	ret = eth_intel_igc_tx_dma_init(dev);
	if (ret) {
		return ret;
	}

	ret = eth_intel_igc_rx_dma_init(dev);
	if (ret) {
		return ret;
	}

	return ret;
}

static const struct ethernet_api eth_api = {
	.iface_api.init      = eth_intel_igc_iface_init,
	.get_capabilities    = eth_intel_igc_get_caps,
	.set_config          = eth_intel_igc_set_config,
	.send                = eth_intel_igc_tx_data,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.get_stats           = eth_intel_igc_get_stats,
#endif
};

#define NUM_QUEUES(n)   DT_INST_PROP(n, num_queues)
#define NUM_TX_DESC(n)  DT_INST_PROP(n, num_tx_desc)
#define NUM_RX_DESC(n)  DT_INST_PROP(n, num_rx_desc)
/* Total MSI-X includes queue-specific and 1 other cause interrupt */
#define NUM_MSIX(n)     NUM_QUEUES(n) + 1

#define INTEL_IGC_MAC_ASSIGN_ADDR(n) \
	BUILD_ASSERT(NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(n)), \
		     "eth_intel_igc requires mac address"); \
	static void assign_mac_addr_##n(uint8_t mac_addr[INTEL_IGC_ALEN]) \
	{ \
		const uint8_t addr[INTEL_IGC_ALEN] = DT_INST_PROP(n, local_mac_address); \
		memcpy(mac_addr, addr, sizeof(addr)); \
	}

#define INTEL_IGC_MAC_CONFIG_IRQ(n) \
	static void eth##n##_cfg_irq(const struct device *dev) \
	{ \
		eth_intel_igc_map_interrupt_to_vector(dev); \
		eth_intel_igc_init_queue_work(dev); \
		eth_intel_igc_init_link_work(dev); \
	}

/**
 * @brief This macro Generete TX and RX interrupt handling functions as per queue.
 */
#define INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, queue) \
	static void eth_tx_irq_queue##n##_##queue(struct k_work *item) \
	{ \
		struct k_work *work = item; \
		struct eth_intel_igc_mac_data *data = \
			CONTAINER_OF(work, struct eth_intel_igc_mac_data, tx_work[queue]); \
		\
		eth_intel_igc_tx_clean(data, queue); \
	} \
	static void eth_rx_irq_queue##n##_##queue(struct k_work *item) \
	{ \
		struct k_work_delayable *dwork = k_work_delayable_from_work(item); \
		struct eth_intel_igc_mac_data *data = \
			CONTAINER_OF(dwork, struct eth_intel_igc_mac_data, rx_work[queue]); \
		\
		eth_intel_igc_rx_data(data, queue); \
	}

#define INTEL_IGC_SETUP_QUEUE_WORK(n) \
	INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, 3) \
	INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, 2) \
	INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, 1) \
	INTEL_IGC_SETUP_QUEUE_WORK_EXP(n, 0)

#define INTEL_IGC_INIT_QUEUE_WORK_EXP(n, queue) \
	k_work_init(&data->tx_work[queue], eth_tx_irq_queue##n##_##queue); \
	k_work_init_delayable(&data->rx_work[queue], eth_rx_irq_queue##n##_##queue); \

/**
 * @brief This macro initializes TX and RX work queue item for each queue.
 */
#define INTEL_IGC_INIT_QUEUE_WORK(n) \
	static void eth_intel_igc_init_queue_work(const struct device *dev) \
	{ \
		struct eth_intel_igc_mac_data *data = dev->data; \
		uint8_t queue = NUM_QUEUES(n); \
		\
		if (queue > 3) { \
			INTEL_IGC_INIT_QUEUE_WORK_EXP(n, 3); \
		} \
		if (queue > 2) { \
			INTEL_IGC_INIT_QUEUE_WORK_EXP(n, 2); \
		} \
		if (queue > 1) { \
			INTEL_IGC_INIT_QUEUE_WORK_EXP(n, 1); \
		} \
		if (queue > 0) { \
			INTEL_IGC_INIT_QUEUE_WORK_EXP(n, 0); \
		} \
	}

/**
 * @brief This macro allocates various global objects required for managing
 * the transmit and receive operations.
 */
#define INTEL_IGC_ALLOC_GLOBAL_OBJECTS(n) \
	struct k_sem tx_desc_ring_lock_##n[NUM_QUEUES(n)]; \
	struct k_sem rx_desc_ring_lock_##n[NUM_QUEUES(n)]; \
	static unsigned int tx_desc_ring_wr_ptr_##n[NUM_QUEUES(n)]; \
	static unsigned int tx_desc_ring_rd_ptr_##n[NUM_QUEUES(n)]; \
	static unsigned int rx_desc_ring_wr_ptr_##n[NUM_QUEUES(n)]; \
	static unsigned int rx_desc_ring_rd_ptr_##n[NUM_QUEUES(n)]; \
	static struct net_buf tx_frag_##n[NUM_QUEUES(n)][NUM_TX_DESC(n)]; \
	static struct net_pkt tx_pkt_##n[NUM_QUEUES(n)][NUM_TX_DESC(n)]; \
	static msi_vector_t msi_vector_##n[NUM_MSIX(n)]; \
	static struct eth_intel_igc_qvec qvec_##n[NUM_QUEUES(n)]; \

/**
 * @brief This macro defines the runtime data structure for each instance of the driver.
 */
#define INTEL_IGC_MAC_DATA(n) \
	static struct eth_intel_igc_mac_data eth_data_##n = { \
		.tx_desc_ring_wr_ptr    = tx_desc_ring_wr_ptr_##n, \
		.rx_desc_ring_wr_ptr    = rx_desc_ring_wr_ptr_##n, \
		.tx_desc_ring_rd_ptr    = tx_desc_ring_rd_ptr_##n, \
		.rx_desc_ring_rd_ptr    = rx_desc_ring_rd_ptr_##n, \
		.tx_desc_ring_lock      = tx_desc_ring_lock_##n, \
		.rx_desc_ring_lock      = rx_desc_ring_lock_##n, \
		.tx_frag                = (struct net_buf **)tx_frag_##n, \
		.tx_pkt                 = (struct net_pkt **)tx_pkt_##n, \
		.vectors                = msi_vector_##n, \
		.q_vector               = qvec_##n, \
	};

/**
 * @brief This macro defines the configuration structure for each instance of the driver.
 */
#define INTEL_IGC_MAC_CONFIG(n) \
	static void eth##n##_cfg_irq(const struct device *dev); \
	static const struct eth_intel_igc_mac_cfg eth_cfg_##n = { \
		.platform               = DEVICE_DT_GET(DT_INST_PARENT(n)), \
		.phy                    = DEVICE_DT_GET(DT_NODELABEL(ethphy##n)), \
		.mac_addr_exist_in_nvm  = DT_INST_PROP(n, mac_addr_exist_in_nvm), \
		.assign_mac_addr        = assign_mac_addr_##n, \
		.config_func            = eth##n##_cfg_irq, \
		.num_queues             = NUM_QUEUES(n), \
		.num_msix               = NUM_MSIX(n), \
		.num_tx_desc            = NUM_TX_DESC(n), \
		.num_rx_desc            = NUM_RX_DESC(n), \
	};

#define INTEL_IGC_MAC_INIT(n) \
	DEVICE_PCIE_INST_DECLARE(n); \
	INTEL_IGC_MAC_ASSIGN_ADDR(n); \
	INTEL_IGC_MAC_CONFIG(n) \
	INTEL_IGC_ALLOC_GLOBAL_OBJECTS(n) \
	INTEL_IGC_MAC_DATA(n) \
	INTEL_IGC_SETUP_QUEUE_WORK(n); \
	INTEL_IGC_INIT_QUEUE_WORK(n); \
	\
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_intel_igc_init, NULL, \
				      &eth_data_##n, &eth_cfg_##n, \
				      CONFIG_ETH_INIT_PRIORITY, \
				      &eth_api, CONFIG_ETH_INTEL_IGC_NET_MTU); \
	INTEL_IGC_MAC_CONFIG_IRQ(n) \

DT_INST_FOREACH_STATUS_OKAY(INTEL_IGC_MAC_INIT)
