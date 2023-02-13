/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2022, Intel Corporation
 * Description:
 * 3504-0 Universal 10/100/1000 Ethernet MAC (DWC_gmac)
 * Driver specifically designed for Cyclone V SoC DevKit use only.
 *
 * based on Intel SOC FPGA HWLIB Repo
 * https://github.com/altera-opensource/intel-socfpga-hwlib
 */

#define LOG_MODULE_NAME eth_cyclonev
#define LOG_LEVEL	CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DT_DRV_COMPAT snps_ethernet_cyclonev

#include "eth_cyclonev_priv.h"

#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>

#include "phy_cyclonev.c"
#include <ethernet/eth_stats.h>
#include <sys/types.h>
#include <zephyr/irq.h>
#define TX_AVAIL_WAIT	    K_MSEC(1)
#define INC_WRAP(idx, size) ({ idx = (idx + 1) % size; })

static const uint8_t eth_cyclonev_mac_addr[6] = DT_INST_PROP(0, local_mac_address);

void eth_cyclonev_reset(uint32_t instance);
void eth_cyclonev_set_mac_addr(uint8_t *address, uint32_t instance, uint32_t n,
				struct eth_cyclonev_priv *p);
int eth_cyclonev_get_software_reset_status(uint32_t instance, struct eth_cyclonev_priv *p);
int eth_cyclonev_software_reset(uint32_t instance, struct eth_cyclonev_priv *p);
void eth_cyclonev_setup_rxdesc(struct eth_cyclonev_priv *p);
void eth_cyclonev_setup_txdesc(struct eth_cyclonev_priv *p);
static void eth_cyclonev_iface_init(struct net_if *iface);
static int eth_cyclonev_send(const struct device *dev, struct net_pkt *pkt);
void eth_cyclonev_isr(const struct device *dev);
int set_mac_conf_status(int instance, uint32_t *mac_config_reg_settings,
				struct eth_cyclonev_priv *p);
int eth_cyclonev_probe(const struct device *dev);
static int eth_cyclonev_start(const struct device *dev);
static int eth_cyclonev_stop(const struct device *dev);
static void eth_cyclonev_receive(struct eth_cyclonev_priv *p);
static void eth_cyclonev_tx_release(struct eth_cyclonev_priv *p);
static int eth_cyclonev_set_config(const struct device *dev, enum ethernet_config_type type,
				const struct ethernet_config *config);
static enum ethernet_hw_caps eth_cyclonev_caps(const struct device *dev);

/** Device config */
struct eth_cyclonev_config {
	/** BBRAM base address */
	uint8_t *base;
	/** BBRAM size (Unit:bytes) */
	int size;
	uint32_t emac_index;
	void (*irq_config)(void);
};

/**
 * @brief Reset gmac device function
 * Function initialise HPS interface, see
 * https://www.intel.com/content/dam/www/programmable
 * /us/en/pdfs/literature/hb/cyclone-v/cv_54001.pdf p. 1252
 *
 * @param instance Number of instance (0 or 1 in Cyclone V HPS)
 */

void eth_cyclonev_reset(uint32_t instance)
{
	/*	 1. After the HPS is released from cold or warm reset,
	 *reset the Ethernet Controller module by setting the appropriate
	 *emac bit in the permodrst register in the Reset Manager.
	 */

	sys_set_bits(RSTMGR_PERMODRST_ADDR, Rstmgr_Permodrst_Emac_Set_Msk[instance]);

	/*	 4a. Set the physel_* field in the ctrl register of the System Manager
	 *(EMAC Group) to 0x1 to select the RGMII PHY interface.
	 */

	alt_replbits_word(SYSMGR_EMAC_ADDR, Sysmgr_Core_Emac_Phy_Intf_Sel_Set_Msk[instance],
			  Sysmgr_Emac_Phy_Intf_Sel_E_Rgmii[instance]);

	/*	 4b. Disable the Ethernet Controller FPGA interfaces by clearing the
	 * emac_* bit in the module register of the System Manager (FPGA Interface
	 * group).
	 */

	sys_clear_bits(SYSMGR_FPGAINTF_INDIV_ADDR, Sysmgr_Fpgaintf_En_3_Emac_Set_Msk[instance]);

	/*	 7. After confirming the settings are valid, software can clear the emac
	 * bit in the permodrst register of the Reset Manager to bring the EMAC out of
	 * reset.
	 */

	sys_clear_bits(RSTMGR_PERMODRST_ADDR, Rstmgr_Permodrst_Emac_Set_Msk[instance]);
}

/**
 * @brief Set MAC Address function
 * Loads the selected MAC Address in device's registers.
 *
 * @param address Pointer to Mac Address table
 * @param instance Number of instance (0 or 1 in Cyclone V HPS)
 * @param n Selected index of MAC Address, n <= 15. There's no implementation
 * of setting MAC Addresses for n > 15.
 *
 */

void eth_cyclonev_set_mac_addr(uint8_t *address, uint32_t instance, uint32_t n,
	struct eth_cyclonev_priv *p)
{
	uint32_t tmpreg;

	if (instance > 1) {
		return;
	}
	if (n > 15) {
		LOG_ERR("Invalid index of MAC Address: %d", n);
		return;
	}

	/* Calculate the selected MAC address high register */
	tmpreg = ((uint32_t)address[5] << 8) | (uint32_t)address[4];

	/* Load the selected MAC address high register */
	sys_write32(tmpreg, EMAC_GMAC_MAC_ADDR_HIGH_ADDR(p->base_addr, n));

	/* Calculate the selected MAC address low register */
	tmpreg = ((uint32_t)address[3] << 24) | ((uint32_t)address[2] << 16) |
		 ((uint32_t)address[1] << 8) | address[0];

	/* Load the selected MAC address low register */
	sys_write32(tmpreg, EMAC_GMAC_MAC_ADDR_LOW_ADDR(p->base_addr, n));
}

/**
 * @brief Get software reset status function
 * Check status of SWR bit in DMA Controller Bus_Mode Register
 *
 * @param instance Number of instance (0 or 1 in Cyclone V HPS)
 * @retval 1 if DMA Controller Resets Logic, 0 otherwise
 */

int eth_cyclonev_get_software_reset_status(uint32_t instance, struct eth_cyclonev_priv *p)
{
	if (instance > 1) {
		return -1;
	}
	return EMAC_DMA_MODE_SWR_GET(sys_read32(EMAC_DMAGRP_BUS_MODE_ADDR(p->base_addr)));
}

/**
 * @brief Perform software reset
 * Resets all MAC subsystem registers and logic, wait for the software reset to
 * clear
 *
 * @param instance Number of instance (0 or 1 in Cyclone V HPS)
 * @retval 0 if Reset was successful, -1 otherwise
 */

int eth_cyclonev_software_reset(uint32_t instance, struct eth_cyclonev_priv *p)
{
	unsigned int i;

	if (instance > 1) {
		return -1;
	}

	/* Set the SWR bit: resets all MAC subsystem internal registers and logic */
	/* After reset all the registers holds their respective reset values */
	sys_set_bits(EMAC_DMAGRP_BUS_MODE_ADDR(p->base_addr), EMAC_DMA_MODE_SWR_SET_MSK);

	/* Wait for the software reset to clear */
	for (i = 0; i < 10; i++) {
		k_sleep(K_MSEC(10));
		if (eth_cyclonev_get_software_reset_status(instance, p) == 0) {
			break;
		}
	}

	if (i == 10) {
		return -1;
	}

	return 0;
}

/**
 * @brief RX descriptor ring initialisation function
 * Sets up RX descriptor ring with chained descriptors,
 * sets OWN bit in each descriptor, inits rx variables and stats
 *
 * @param p Pointer to device structure.
 */

void eth_cyclonev_setup_rxdesc(struct eth_cyclonev_priv *p)
{
	int32_t i;
	struct eth_cyclonev_dma_desc *rx_desc;

	/* For each descriptor where i = descriptor index do: */
	for (i = 0; i < NB_RX_DESCS; i++) {
		rx_desc = &p->rx_desc_ring[i];
		rx_desc->buffer1_addr = (uint32_t)&p->rx_buf[i * ETH_BUFFER_SIZE];
		rx_desc->control_buffer_size = ETH_DMARXDESC_RCH | ETH_BUFFER_SIZE;

		/*set own bit*/
		rx_desc->status = ETH_DMARXDESC_OWN;

		rx_desc->buffer2_next_desc_addr = (uint32_t)&p->rx_desc_ring[i + 1];
		if (i == (NB_RX_DESCS - 1)) {
			rx_desc->buffer2_next_desc_addr = (uint32_t)&p->rx_desc_ring[0];
		}
	}

	p->rx_current_desc_number = 0;
	p->rxints = 0;

	/* Set RX Descriptor List Address Register */
	sys_write32((uint32_t)&p->rx_desc_ring[0],
		    EMAC_DMA_RX_DESC_LIST_ADDR(p->base_addr));
}

/**
 * @brief TX descriptor ring initialisation function
 * Sets up TX descriptor ring with chained descriptors,
 * sets OWN bit in each descriptor, inits rx variables and stats
 *
 * @param p Pointer to device structure.
 */

void eth_cyclonev_setup_txdesc(struct eth_cyclonev_priv *p)
{
	int32_t i;

	struct eth_cyclonev_dma_desc *tx_desc;

	/* For each descriptor where i = descriptor index do: */
	for (i = 0; i < NB_TX_DESCS; i++) {
		tx_desc = &p->tx_desc_ring[i];
		tx_desc->buffer1_addr = (uint32_t)&p->tx_buf[i * ETH_BUFFER_SIZE];
		tx_desc->buffer2_next_desc_addr = (uint32_t)&p->tx_desc_ring[i + 1];
		tx_desc->status = 0;
		tx_desc->control_buffer_size = 0;

		if (i == (NB_TX_DESCS - 1)) {
			tx_desc->buffer2_next_desc_addr = (uint32_t)&p->tx_desc_ring[0];
		}
	}

	p->tx_current_desc_number = 0;
	p->txints = 0;
	p->tx_tail = 0;

	/* Set TX Descriptor List Address Register */
	sys_write32((uint32_t)&p->tx_desc_ring[0],
		    EMAC_DMA_TX_DESC_LIST_ADDR(p->base_addr));
}

/**
 * @brief Ethernet interface initialisation function
 * Inits interface, sets interface link MAC address
 *
 * @param iface Pointer to net_if structure
 */

/* Initialisation of interface */
static void eth_cyclonev_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_cyclonev_config *config = dev->config;
	struct eth_cyclonev_priv *p = dev->data;

	p->iface = iface;
	ethernet_init(iface);
	net_if_set_link_addr(iface, p->mac_addr, sizeof(p->mac_addr), NET_LINK_ETHERNET);

	/*
	 * Semaphores are used to represent number of available descriptors.
	 * The total is one less than ring size in order to always have
	 * at least one inactive slot for the hardware tail pointer to
	 * stop at and to prevent our head indexes from looping back
	 * onto our tail indexes.
	 */
	k_sem_init(&p->free_tx_descs, NB_TX_DESCS - 1, NB_TX_DESCS - 1);

	/* Initialize the ethernet irq handler */
	config->irq_config();

	p->initialised = 1;
	LOG_DBG("done");
}

/**
 * @brief Ethernet set config function usually called by
 *	Zephyr Ethernet stack. It supports currently two options:
 *	Set of Mac address and Enabling Promiscuous Mode
 *
 * @param dev Pointer to net_if structure
 * @param type Enumerated type of configuration to do
 * @param config Pointer to ethernet_config structure
 * @retval ret 0 if successful
 */

static int eth_cyclonev_set_config(const struct device *dev, enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	struct eth_cyclonev_priv *p = dev->data;
	const struct eth_cyclonev_config *cv_config = dev->config;
	uint32_t reg_val;
	int ret = 0;

	(void)reg_val; /* silence the "unused variable" warning */

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(p->mac_addr, config->mac_address.addr, sizeof(p->mac_addr));
		eth_cyclonev_set_mac_addr(p->mac_addr, cv_config->emac_index, 0, p); /* Set MAC */
		net_if_set_link_addr(p->iface, p->mac_addr, sizeof(p->mac_addr), NET_LINK_ETHERNET);
		break;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		reg_val = sys_read32(EMAC_GMACGRP_MAC_FRAME_FILTER_ADDR(p->base_addr));
		if (config->promisc_mode && !(reg_val & EMAC_GMACGRP_MAC_FRAME_FILTER_PR_SET_MSK)) {
			/* Turn on Promisc Mode */
			sys_set_bits(EMAC_GMACGRP_MAC_FRAME_FILTER_ADDR(p->base_addr),
				     EMAC_GMACGRP_MAC_FRAME_FILTER_PR_SET_MSK);
		} else if (!config->promisc_mode &&
			   (reg_val & EMAC_GMACGRP_MAC_FRAME_FILTER_PR_SET_MSK)) {
			/* Turn off Promisc Mode */
			sys_clear_bits(EMAC_GMACGRP_MAC_FRAME_FILTER_ADDR(p->base_addr),
				       EMAC_GMACGRP_MAC_FRAME_FILTER_PR_SET_MSK);
		} else {
			ret = -EALREADY;
		}
		break;
#endif
	default:
		ret = -ENOTSUP;
		break;
	}
	LOG_DBG("set_config: ret = %d ", ret);
	return ret;
}

/**
 * @brief Get capabilities function usually called by
 *	Zephyr Ethernet stack.
 *
 * @param dev Pointer to net_if structure
 * @retval caps Enumerated capabilities of device
 */

static enum ethernet_hw_caps eth_cyclonev_caps(const struct device *dev)
{
	struct eth_cyclonev_priv *p = dev->data;
	enum ethernet_hw_caps caps = 0;

	if (p->feature & EMAC_DMA_HW_FEATURE_MIISEL) {
		caps |= ETHERNET_LINK_10BASE_T;
		caps |= ETHERNET_LINK_100BASE_T;
	}
	if (p->feature & EMAC_DMA_HW_FEATURE_GMIISEL) {
		caps |= ETHERNET_LINK_1000BASE_T;
	}
	if (p->feature & EMAC_DMA_HW_FEATURE_RXTYP2COE) {
		caps |= ETHERNET_HW_RX_CHKSUM_OFFLOAD;
	}
	if (p->feature & EMAC_DMA_HW_FEATURE_RXTYP1COE) {
		caps |= ETHERNET_HW_RX_CHKSUM_OFFLOAD;
	}

	caps |= ETHERNET_PROMISC_MODE;

	return caps;
}

/**
 * @brief Send packet function
 * Sends packet of data. See:
 * https://www.intel.com/content/dam/www/programmable/us/en/pdfs/
 * literature/hb/cyclone-v/cv_54001.pdf p.1254 and p.1206
 *
 * @param dev Pointer to device structure
 * @param pkt Pointer to net_pkt structure containing packet to sent
 * @retval 0 if successful, -1 otherwise
 */

static int eth_cyclonev_send(const struct device *dev, struct net_pkt *pkt)
{
	LOG_DBG("ethernet CVSX sending...\n");

	struct eth_cyclonev_priv *p = dev->data;
	struct eth_cyclonev_dma_desc *tx_desc;
	int32_t index = 0;
	uint16_t len = net_pkt_get_len(pkt);
	int first = 1;
	struct net_buf *frag;

	LOG_DBG("Pkt length: %d", len);
	frag = pkt->buffer;
	do {

		/* reserve a free descriptor for this fragment */
		if (k_sem_take(&p->free_tx_descs, TX_AVAIL_WAIT) != 0) {
			LOG_DBG("no more free tx descriptors");
			goto abort;
		}

		tx_desc = &p->tx_desc_ring[p->tx_current_desc_number];

		/* Check if it is a free descriptor.  */
		if (tx_desc->status & ETH_DMATXDESC_OWN) {
			/* Buffer is still owned by device.  */
			LOG_ERR("No free tx descriptors!\n");
			goto abort;
		}

		/* check if len is too large */
		if (len >= ETH_BUFFER_SIZE) {
			LOG_ERR("Length of packet is too long\n");
			goto abort;
		}

		/* Copy data to local buffer   */

		if (frag) {
			memcpy(&p->tx_buf[p->tx_current_desc_number * ETH_BUFFER_SIZE], frag->data,
			       len);
		}

		/* Set the buffer size.  */
		tx_desc->control_buffer_size = (frag->len & ETH_DMATXDESC_TBS1);

		LOG_DBG("Desc[%d] at address: 0x%08x: , Frag size: %d, Buffer Addr: %p",
			p->tx_current_desc_number,
			(unsigned int)&p->tx_desc_ring[p->tx_current_desc_number], frag->len,
			(void *)tx_desc->buffer1_addr);

		tx_desc->status = ETH_DMATXDESC_TCH;

		/* Set the Descriptor's FS bit.  */
		if (first) {
			tx_desc->status |= (ETH_DMATXDESC_FS | ETH_DMATXDESC_CIC_BYPASS);
			first = 0;
		}

		/* If Last: then (...) */
		if (!frag->frags) {
			/* set the Descriptor's LS and IC bit.  */
			tx_desc->status |= (ETH_DMATXDESC_LS | ETH_DMATXDESC_IC);
			index = p->tx_current_desc_number;
		}

		/* Set the current index to the next descriptor.  */
		p->tx_current_desc_number = (p->tx_current_desc_number + 1);
		if (p->tx_current_desc_number >= NB_TX_DESCS) {
			p->tx_current_desc_number = 0;
		}

		if (!frag->frags) {

			while (1) {

				tx_desc = &p->tx_desc_ring[index];

				if (tx_desc->status & ETH_DMATXDESC_OWN) {
					LOG_ERR("Send packet error!\n");
					/* Restart DMA transmission and re-initialise
					 * TX descriptors
					 */
					sys_clear_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(
							       p->base_addr),
						       EMAC_DMAGRP_OPERATION_MODE_ST_SET_MSK);
					sys_set_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(
							     p->base_addr),
						     EMAC_DMAGRP_OPERATION_MODE_FTF_SET_MSK);
					eth_cyclonev_setup_txdesc(p);
					sys_set_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(
							     p->base_addr),
						     EMAC_DMAGRP_OPERATION_MODE_ST_SET_MSK);
					goto abort;
				}

				/* Set OWN bit.  */
				tx_desc->status |= ETH_DMATXDESC_OWN;

				if (tx_desc->status & ETH_DMATXDESC_FS) {
					break;
				}

				index--;
				if (index < 0) {
					index = NB_TX_DESCS - 1;
				}
			}

			LOG_DBG("Current Host Transmit Descriptor Register: 0x%08x",
				sys_read32(
					EMAC_DMA_CURR_HOST_TX_DESC_ADDR(p->base_addr)));
			LOG_DBG("Current Host Transmit Buffer Register: 0x%08x",
				sys_read32(
					EMAC_DMA_CURR_HOST_TX_BUFF_ADDR(p->base_addr)));

			/* If the DMA transmission is suspended, resume transmission.  */
			if (sys_read32(EMAC_DMAGRP_STATUS_ADDR(p->base_addr)) &
			    EMAC_DMAGRP_STATUS_TS_SET_MSK) {

				/* Clear TBUS ETHERNET DMA flag */
				sys_write32(EMAC_DMAGRP_STATUS_TS_SET_MSK,
					    EMAC_DMAGRP_STATUS_ADDR(p->base_addr));

				/* Resume DMA transmission */
				sys_write32(0,
					    EMAC_DMA_TX_POLL_DEMAND_ADDR(p->base_addr));
			}
		}
		frag = frag->frags;
	} while (frag);

	LOG_DBG("Sent");
	return 0;

abort:
	k_sem_give(&p->free_tx_descs); /* Multi-descriptor package release (?) */

	return -1;
}

/**
 * @brief Interrupt handling function
 * Detects interrupt status, invokes necessary actions
 * and clears interrupt status register
 *
 * @param dev Pointer to device structure
 */

void eth_cyclonev_isr(const struct device *dev)
{
	struct eth_cyclonev_priv *p = dev->data;
	const struct eth_cyclonev_config *config = dev->config;
	uint32_t irq_status = 0;
	uint32_t irq_status_emac = 0;

	irq_status =
		sys_read32(EMAC_DMAGRP_STATUS_ADDR(p->base_addr)) & p->interrupt_mask;
	irq_status_emac = sys_read32(EMAC_GMAC_INT_STAT_ADDR(p->base_addr));
	LOG_DBG("DMA_IRQ_STATUS = 0x%08x, emac: 0x%08x", irq_status, irq_status_emac);

	if (irq_status & EMAC_DMA_INT_EN_NIE_SET_MSK) {
		sys_write32(EMAC_DMA_INT_EN_NIE_SET_MSK,
			    EMAC_DMAGRP_STATUS_ADDR(p->base_addr));
	}

	if (irq_status & EMAC_DMA_INT_EN_TIE_SET_MSK) {
		p->txints++;
		eth_cyclonev_tx_release(p);
		/* Clear the selected ETHERNET DMA bit(s) */
		sys_write32(EMAC_DMA_INT_EN_TIE_SET_MSK,
			    EMAC_DMAGRP_STATUS_ADDR(p->base_addr));
	}

	if (irq_status & EMAC_DMA_INT_EN_RIE_SET_MSK) {
		p->rxints++;
		eth_cyclonev_receive(p);
		/* Clear the selected ETHERNET DMA bit(s) */
		sys_write32(EMAC_DMA_INT_EN_RIE_SET_MSK,
			    EMAC_DMAGRP_STATUS_ADDR(p->base_addr));
	}

	if (irq_status_emac & EMAC_GMAC_INT_STAT_RGSMIIIS_SET_MSK) {
		/* Clear the selected ETHERNET GMAC bit(s) */
		uint32_t regval = sys_read32(GMACGRP_CONTROL_STATUS_ADDR(p->base_addr));

		if (EMAC_GMAC_MII_CTL_STAT_LNKSTS_GET(regval)) {
			LOG_INF("Link is up");
		} else {
			LOG_INF("Link is down");
			return;
		}

		if (EMAC_GMAC_MII_CTL_STAT_LNKMOD_GET(regval)) {
			LOG_INF("Full duplex");
		} else {
			LOG_INF("Half duplex");
		}

		switch (EMAC_GMAC_MII_CTL_STAT_LNKSPEED_GET(regval)) {
		case 0:
			LOG_INF("Link Speed 2.5MHz");
			break;
		case 1:
			LOG_INF("Link Speed 25MHz");
			break;
		case 2:
			LOG_INF("Link Speed 125MHz");
			break;
		default:
			LOG_ERR("LNKSPEED_GET_ERROR");
			break;
		}

		if (p->initialised) {
			uint32_t cfg_reg_set;

			cfg_reg_set = sys_read32(GMACGRP_MAC_CONFIG_ADDR(p->base_addr));

			if (eth_cyclonev_stop(dev) == -1) {
				LOG_ERR("Couldn't stop device: %s", dev->name);
				return;
			}

			set_mac_conf_status(config->emac_index, &cfg_reg_set, p);
			sys_write32(cfg_reg_set, GMACGRP_MAC_CONFIG_ADDR(p->base_addr));

			eth_cyclonev_start(dev);
		}
	}
}

/**
 * @brief Receive packet function (IRQ)
 * In the event of receive completion interrupt, this function
 * copies data from buffer to necessary net stack structures
 * performs general error checking and returns descriptor to hardware.
 *
 * @param p Pointer to device structure
 *
 */

static void eth_cyclonev_receive(struct eth_cyclonev_priv *p)
{
	struct eth_cyclonev_dma_desc *rx_desc;
	struct net_pkt *pkt;
	uint32_t index, frame_length, rx_search, wrap, data_remaining, last_desc_index, buf_size;

	index = p->rx_current_desc_number;
	rx_desc = &p->rx_desc_ring[index];

	while (!(rx_desc->status & ETH_DMARXDESC_OWN)) {

		LOG_DBG("RDES0[%d] = 0x%08x", index, rx_desc->status);
		/* Look for FS bit */
		if (!(rx_desc->status & ETH_DMARXDESC_FS)) {
			LOG_ERR("Unexpected missing FS bit");
			rx_desc->status |= ETH_DMARXDESC_OWN;
			goto cont;
		}
		/* Look for EOF bit, save frame length including multiple
		 * buffers and index of last descriptor
		 */
		rx_search = index;
		wrap = index;
		do {
			rx_desc = &p->rx_desc_ring[rx_search];
			/* Frame length */
			frame_length = data_remaining = (ETH_DMARXDESC_FL & rx_desc->status) >> 16;
			last_desc_index = rx_search;
			if (!(rx_desc->status & ETH_DMARXDESC_LS)) {
				INC_WRAP(rx_search, NB_RX_DESCS);
				if (rx_search == wrap) {
					LOG_ERR("Couldn't find EOF bit!");
					rx_desc = &p->rx_desc_ring[index];
					rx_desc->status |= ETH_DMARXDESC_OWN;
					goto cont;
				}
			}
		} while (!(rx_desc->status & ETH_DMARXDESC_LS));

		LOG_DBG("Frame length = %d, Last descriptor = %d", frame_length, last_desc_index);
		p->rx_current_desc_number = last_desc_index;

		/* Allocate packet with buffer */
		pkt = net_pkt_rx_alloc_with_buffer(p->iface, frame_length, AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("net_pkt_rx_alloc_with_buffer() failed");
			eth_stats_update_errors_rx(p->iface);
		}

		/* Copy data from multiple buffers and descriptors */
		rx_search = index;
		wrap = index;
		do {
			rx_desc = &p->rx_desc_ring[rx_search];
			if (data_remaining < ETH_BUFFER_SIZE) {
				buf_size = data_remaining;
			} else {
				buf_size = ETH_BUFFER_SIZE;
			}
			if (pkt) {
				net_pkt_write(pkt, &p->rx_buf[rx_search * ETH_BUFFER_SIZE],
					      buf_size);
			}
			data_remaining -= buf_size;
			rx_desc->status |= ETH_DMARXDESC_OWN;
			if (last_desc_index != rx_search) {
				INC_WRAP(rx_search, NB_RX_DESCS);
				if (rx_search == wrap) {
					LOG_ERR("Couldn't find last descriptor! Data remaining: %d",
						data_remaining);
					goto cont;
				}
				if (rx_search == last_desc_index) {
					/* One more iteration */
					rx_desc = &p->rx_desc_ring[rx_search];
					if (data_remaining < ETH_BUFFER_SIZE) {
						buf_size = data_remaining;
					} else {
						buf_size = ETH_BUFFER_SIZE;
					}
					if (pkt) {
						net_pkt_write(
							pkt,
							&p->rx_buf[rx_search * ETH_BUFFER_SIZE],
							buf_size);
					}
					data_remaining -= buf_size;

					rx_desc->status |= ETH_DMARXDESC_OWN;
				}
			}
		} while (last_desc_index != rx_search);

		/* Hand-over packet into IP stack */
		if (pkt) {
			if (net_recv_data(p->iface, pkt) < 0) {
				LOG_ERR("RX packet hand-over to IP stack failed");
				net_pkt_unref(pkt);
			}
			LOG_DBG("Received packet %p, len %d", pkt, frame_length);
		}

cont:
		p->rx_current_desc_number++;
		if (p->rx_current_desc_number == NB_RX_DESCS) {
			p->rx_current_desc_number = 0;
		}
		index = p->rx_current_desc_number;
		rx_desc = &p->rx_desc_ring[index];
	}
}

/**
 * @brief Release tx function
 * Main purpose of its function is to track current descriptor number
 * and give back succeding tx semaphore when it have been used.
 *
 * @param p Pointer to device structure
 */

static void eth_cyclonev_tx_release(struct eth_cyclonev_priv *p)
{
	unsigned int d_idx;
	struct eth_cyclonev_dma_desc *d;
	uint32_t des3_val;

	for (d_idx = p->tx_tail; d_idx != p->tx_current_desc_number;
	     INC_WRAP(d_idx, NB_TX_DESCS), k_sem_give(&p->free_tx_descs)) {

		d = &p->tx_desc_ring[d_idx];
		des3_val = d->status;
		LOG_DBG("TDES3[%d] = 0x%08x", d_idx, des3_val);

		/* stop here if hardware still owns it */
		if (des3_val & ETH_DMATXDESC_OWN) {
			break;
		}

		/* last packet descriptor: */
		if (des3_val & ETH_DMATXDESC_LS) {
			/* log any errors */
			if (des3_val & ETH_DMATXDESC_ES) {
				LOG_ERR("tx error (DES3 = 0x%08x)", des3_val);
				eth_stats_update_errors_tx(p->iface);
			}
		}
	}
	p->tx_tail = d_idx;
}

/**
 * @brief Sets MAC Config Register (not implemented)
 * Detects PHY Mode and assigns MAC Config Register
 *
 * @param instance Number of instance (0 or 1 in Cyclone V HPS)
 * @param mac_config_reg_settings Mac_config register mask to set
 * @retval updated mac_config_reg mask (>=0), -1 otherwise
 */
/* Configure the MAC with the speed fixed by the auto-negotiation process */
int set_mac_conf_status(int instance, uint32_t *mac_config_reg_settings,
	struct eth_cyclonev_priv *p)
{
	uint16_t phy_duplex_status, phy_speed;
	int ret;

	ret = alt_eth_phy_get_duplex_and_speed(&phy_duplex_status, &phy_speed, instance, p);
	if (ret != 0) {
		LOG_ERR("alt_eth_phy_get_duplex_and_speed failure!");
		return ret;
	}

	/* Set Ethernet duplex mode to Full-duplex following the auto-negotiation */
	if (phy_duplex_status != 0) {
		*mac_config_reg_settings |= EMAC_GMACGRP_MAC_CONFIGURATION_DM_SET_MSK;
	}
	/* Set Ethernet duplex mode to Half-duplex following the auto-negotiation */
	else {
		*mac_config_reg_settings &= ~EMAC_GMACGRP_MAC_CONFIGURATION_DM_SET_MSK;
	}

	/* Set Ethernet speed to 10M following the auto-negotiation */
	if (phy_speed == 10) {
		*mac_config_reg_settings &= ~EMAC_GMACGRP_MAC_CONFIGURATION_FES_SET_MSK;
		*mac_config_reg_settings |= EMAC_GMACGRP_MAC_CONFIGURATION_PS_SET_MSK;
	}

	/* Set Ethernet speed to 100M following the auto-negotiation */
	if (phy_speed == 100) {
		*mac_config_reg_settings |= EMAC_GMACGRP_MAC_CONFIGURATION_FES_SET_MSK;
		*mac_config_reg_settings |= EMAC_GMACGRP_MAC_CONFIGURATION_PS_SET_MSK;
	}

	/* Set Ethernet speed to 1G following the auto-negotiation */
	if (phy_speed == 1000) {
		*mac_config_reg_settings &= ~EMAC_GMACGRP_MAC_CONFIGURATION_PS_SET_MSK;
	}

	return 0;
}

/**
 * @brief Hardware initialisation function
 * Performs EMAC HPS interface initialisation, DMA initialisation,
 * EMAC initialisation and configuration. See:
 * https://www.intel.com/content/dam/
 * www/programmable/us/en/pdfs/literature/hb/cyclone-v/cv_54001.pdf p.1252-54
 *
 * @param dev Pointer to device structure
 * @retval 0 if successful, -1 otherwise
 */

int eth_cyclonev_probe(const struct device *dev)
{
	struct eth_cyclonev_priv *p = dev->data;
	const struct eth_cyclonev_config *config = dev->config;
	uint32_t tmpreg = 0, interrupt_mask;
	uint32_t mac_config_reg_settings = 0;
	int ret;

	p->base_addr = (mem_addr_t)config->base;
	p->running = 0;
	p->initialised = 0;

	/* EMAC HPS Interface Initialization */

	/* Reset the EMAC */
	eth_cyclonev_reset(config->emac_index);

	/* Reset the PHY  */
	ret = alt_eth_phy_reset(config->emac_index, p);
	if (ret != 0) {
		LOG_ERR("alt_eth_phy_reset failure!\n");
		return ret;
	}

	/* Configure the PHY */
	ret = alt_eth_phy_config(config->emac_index, p);
	if (ret != 0) {
		LOG_ERR("alt_eth_phy_config failure!\n");
		return ret;
	}

	/* Read HW feature register */

	p->feature = sys_read32(EMAC_DMA_HW_FEATURE_ADDR(p->base_addr));

	/* DMA Initialisation */

	/* 1. Provide a software reset to reset all of the EMAC internal registers and
	 *logic. (DMA Register 0 (BusMode Register) – bit 0).
	 * 2. Wait for the completion of the reset process (poll bit 0 of the DMA
	 *Register 0 (Bus Mode Register), which is only cleared after the reset
	 *operation is completed).
	 */

	ret = eth_cyclonev_software_reset(config->emac_index, p);
	if (ret != 0) {
		LOG_ERR("eth_cyclonev_software_reset failure!\n");
		return ret;
	}

	/* 4. Program the following fields to initialize the Bus Mode Register by
	 * setting values in DMA Register 0 (Bus Mode Register):
	 */

	sys_write32((tmpreg | EMAC_DMA_MODE_FB_SET_MSK /* Fixed Burst */
		     ),
		    EMAC_DMAGRP_BUS_MODE_ADDR(p->base_addr));

	/*	 5. Program the interface options in Register 10 (AXI Bus Mode
	 * Register). If fixed burst-length is enabled, then select the maximum
	 * burst-length possible on the bus (bits[7:1]).(58)
	 */

	tmpreg = sys_read32(EMAC_DMAGRP_AXI_BUS_MODE_ADDR(p->base_addr));

	sys_write32(
		tmpreg | EMAC_DMAGRP_AXI_BUS_MODE_BLEN16_SET_MSK,
		EMAC_DMAGRP_AXI_BUS_MODE_ADDR(p->base_addr)); /* Set Burst Length = 16 */

	/* 6. Create a proper descriptor chain for transmit and receive. In addition,
	 * ensure that the receive descriptors are owned by DMA (bit 31 of descriptor
	 * should be set).
	 * 7. Make sure that your software creates three or more different transmit or
	 * receive descriptors in the chain before reusing any of the descriptors
	 * 8. Initialize receive and transmit descriptor list address with the base
	 * address of the transmit and receive descriptor (Register 3 (Receive
	 * Descriptor List Address Register) and Register 4 (Transmit Descriptor List
	 * Address Register) respectively).
	 */

	eth_cyclonev_setup_rxdesc(p);
	eth_cyclonev_setup_txdesc(p);

	/*	9. Program the following fields to initialize the mode of operation in
	 * Register 6 (Operation Mode Register):
	 */

	sys_write32((0 | EMAC_DMAGRP_OPERATION_MODE_TSF_SET_MSK /* Transmit Store and Forward */
		     | EMAC_DMAGRP_OPERATION_MODE_RSF_SET_MSK	/* Receive Store and Forward */
		     | EMAC_DMAGRP_OPERATION_MODE_FTF_SET_MSK	/* Receive Store and Forward */
		     ),
		    EMAC_DMAGRP_OPERATION_MODE_ADDR(p->base_addr));

	/*	10.Clear the interrupt requests, by writing to those bits of the status
	 * register (interrupt bits only) that are set. For example, by writing 1 into
	 * bit 16, the normal interrupt summary clears this bit (DMA Register 5
	 * (Status Register)).
	 */

	interrupt_mask = EMAC_DMA_INT_EN_NIE_SET_MSK | EMAC_DMA_INT_EN_RIE_SET_MSK |
			 EMAC_DMA_INT_EN_TIE_SET_MSK;

	p->interrupt_mask = interrupt_mask;

	/* Clear the selected ETHERNET DMA bit(s) */
	sys_write32(interrupt_mask, EMAC_DMAGRP_STATUS_ADDR(p->base_addr));

	/*	11.Enable the interrupts by programming Register 7 (Interrupt Enable
	 * Register).
	 */

	sys_set_bits(EMAC_DMA_INT_EN_ADDR(p->base_addr), interrupt_mask);

	/* 12.Read Register 11 (AHB or AXI Status) to confirm that
	 * all previous transactions are complete.
	 */

	if (sys_read32(EMAC_DMAGRP_AHB_OR_AXI_STATUS_ADDR(p->base_addr)) != 0) {
		LOG_ERR("AHB_OR_AXI_STATUS Fail!\n");
		return -1;
	}

	/* EMAC Initialization and Configuration */

	/*	1. Program the GMII Address Register (offset 0x10) for controlling the
	 * management cycles for theexternal PHY. Bits[15:11] of the GMII Address
	 * Register are written with the Physical Layer Address of the PHY before
	 * reading or writing. Bit 0 indicates if the PHY is busy and is set before
	 * reading or writing to the PHY management interface.
	 * 2. Read the 16-bit data of the GMII Data Register from the PHY for link up,
	 * speed of operation, and mode of operation, by specifying the appropriate
	 * address value in bits[15:11] of the GMII Address Register.
	 */

	mac_config_reg_settings = (EMAC_GMACGRP_MAC_CONFIGURATION_IPC_SET_MSK
				   /* Checksum Offload */
				   | EMAC_GMACGRP_MAC_CONFIGURATION_JD_SET_MSK
				   /* Jabber Disable */
				   | EMAC_GMACGRP_MAC_CONFIGURATION_BE_SET_MSK
				   /* Frame Burst Enable */
				   | EMAC_GMACGRP_MAC_CONFIGURATION_WD_SET_MSK
				   /* Watchdog Disable */
				   | EMAC_GMACGRP_MAC_CONFIGURATION_TC_SET_MSK
				   /* Enable Transmission to PHY */
	);

	ret = set_mac_conf_status(config->emac_index, &mac_config_reg_settings, p);
	if (ret != 0) {
		return -1;
	}

	/*	3. Provide the MAC address registers (MAC Address0 High Register
	 * through MAC Address15 High Register and MAC Address0 Low Register
	 * through MAC Address15 Low Register).
	 */

	memcpy(p->mac_addr, eth_cyclonev_mac_addr, sizeof(p->mac_addr));
	eth_cyclonev_set_mac_addr(p->mac_addr, config->emac_index, 0, p);

	/* 5. Program the following fields to set the appropriate filters for the
	 * incoming frames in the MAC Frame Filter Register:
	 * • Receive All
	 * • Promiscuous mode
	 * • Hash or Perfect Filter
	 * • Unicast, multicast, broadcast, and control frames filter settings
	 */

	sys_clear_bits(EMAC_GMACGRP_MAC_FRAME_FILTER_ADDR(p->base_addr),
		       EMAC_GMACGRP_MAC_FRAME_FILTER_PR_SET_MSK); /* Disable promiscuous mode */

	/*	7. Program the Interrupt Mask Register bits,
	 * as required and if applicable for your configuration.
	 */

	sys_set_bits(EMAC_GMAC_INT_MSK_ADDR(p->base_addr),
		     EMAC_GMAC_INT_STAT_LPIIS_SET_MSK |	       /* Disable Low Power IRQ */
			     EMAC_GMAC_INT_STAT_TSIS_SET_MSK); /* Disable Timestamp IRQ */

	/* 8. Program the appropriate fields in MAC Configuration Register to
	 * configure receive and transmit operation modes...
	 */

	sys_write32(mac_config_reg_settings, GMACGRP_MAC_CONFIG_ADDR(p->base_addr));

	LOG_DBG("func_eth_cyclonev_probe Success!\n");
	return 0;
}

/**
 * @brief Start device function
 * Starts DMA and EMAC transmitter and receiver. See:
 * https://www.intel.com/content/dam/
 * www/programmable/us/en/pdfs/literature/hb/cyclone-v/cv_54001.pdf p.1255-56
 *
 * @param dev Pointer to device structure
 * @retval 0
 */

static int eth_cyclonev_start(const struct device *dev)
{

	struct eth_cyclonev_priv *p = dev->data;

	if (p->running) {
		LOG_DBG("Device already running!");
		return 0;
	}

	/*6. To re-start the operation, first start the DMA and then enable
	 * the EMAC transmitter and receiver.
	 */

	/* Start the DMA */
	sys_set_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(p->base_addr),
		     EMAC_DMAGRP_OPERATION_MODE_ST_SET_MSK);
	sys_set_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(p->base_addr),
		     EMAC_DMAGRP_OPERATION_MODE_SR_SET_MSK);

	/* Enable the EMAC transmitter and receiver */
	sys_set_bits(GMACGRP_MAC_CONFIG_ADDR(p->base_addr),
		     EMAC_GMACGRP_MAC_CONFIGURATION_TE_SET_MSK);
	sys_set_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(p->base_addr),
		     EMAC_DMAGRP_OPERATION_MODE_FTF_SET_MSK); /* Flush Transmit FIFO */
	sys_set_bits(GMACGRP_MAC_CONFIG_ADDR(p->base_addr),
		     EMAC_GMACGRP_MAC_CONFIGURATION_RE_SET_MSK);

	p->running = 1;
	LOG_DBG("Starting Device...");
	return 0;
}

/**
 * @brief Stop device function
 * Stops DMA and EMAC transmitter and receiver. See:
 * https://www.intel.com/content/dam/www/
 * programmable/us/en/pdfs/literature/hb/cyclone-v/cv_54001.pdf p.1255-56
 *
 * @param dev Pointer to device structure
 * @retval 0 if successful, -1 otherwise
 */

static int eth_cyclonev_stop(const struct device *dev)
{

	struct eth_cyclonev_priv *p = dev->data;

	if (!p->running) {
		LOG_DBG("Device is not running!");
		return 0;
	}
	/* 1. Disable the transmit DMA (if applicable), by clearing bit 13
	 * (Start or Stop Transmission Command) of Register 6 (Operation Mode
	 * Register).
	 */

	sys_clear_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(p->base_addr),
		       EMAC_DMAGRP_OPERATION_MODE_ST_SET_MSK);

	/* 3. Disable the EMAC transmitter and EMAC receiver by clearing Bit 3
	 * (TE) and Bit 2 (RE) in Register 0 (MAC Configuration Register).
	 */
	sys_clear_bits(GMACGRP_MAC_CONFIG_ADDR(p->base_addr),
		       EMAC_GMACGRP_MAC_CONFIGURATION_TE_SET_MSK);
	sys_set_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(p->base_addr),
		     EMAC_DMAGRP_OPERATION_MODE_FTF_SET_MSK); /* Flush Transmit FIFO */
	sys_clear_bits(GMACGRP_MAC_CONFIG_ADDR(p->base_addr),
		       EMAC_GMACGRP_MAC_CONFIGURATION_RE_SET_MSK);

	/* 4. Disable the receive DMA (if applicable), after making sure that the data
	 * in the RX FIFO buffer is transferred to the system memory
	 * (by reading Register 9 (Debug Register).
	 */

	sys_clear_bits(EMAC_DMAGRP_OPERATION_MODE_ADDR(p->base_addr),
		       EMAC_DMAGRP_OPERATION_MODE_SR_SET_MSK);

	/* 5. Make sure that both the TX FIFO buffer and RX FIFO buffer are empty. */

	if (EMAC_DMAGRP_DEBUG_RXFSTS_GET(
		    sys_read32(EMAC_DMAGRP_DEBUG_ADDR(p->base_addr))) != 0x0) {
		return -1;
	}

	p->running = 0;
	LOG_DBG("Stopping Device...");
	return 0;
}

const struct ethernet_api eth_cyclonev_api = {.iface_api.init = eth_cyclonev_iface_init,
					      .get_capabilities = eth_cyclonev_caps,
					      .send = eth_cyclonev_send,
					      .start = eth_cyclonev_start,
					      .stop = eth_cyclonev_stop,
					      .set_config = eth_cyclonev_set_config};

#define CYCLONEV_ETH_INIT(inst) \
	static struct eth_cyclonev_priv eth_cyclonev_##inst##_data; \
	static void eth_cyclonev_##inst##_irq_config(void); \
 \
	static const struct eth_cyclonev_config eth_cyclonev_##inst##_cfg = { \
			.base = (uint8_t *)(DT_INST_REG_ADDR(inst)), \
			.size = DT_INST_REG_SIZE(inst), \
			.emac_index = DT_INST_PROP(inst, emac_index), \
			.irq_config = eth_cyclonev_##inst##_irq_config, \
	}; \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, eth_cyclonev_probe, NULL, \
			&eth_cyclonev_##inst##_data, \
			&eth_cyclonev_##inst##_cfg, \
			CONFIG_ETH_INIT_PRIORITY, \
			&eth_cyclonev_api, \
			NET_ETH_MTU); \
 \
	static void eth_cyclonev_##inst##_irq_config(void) \
	{ \
		IRQ_CONNECT(DT_INST_IRQN(inst), \
			    DT_INST_IRQ(inst, priority), eth_cyclonev_isr, \
			    DEVICE_DT_INST_GET(inst), \
			    0); \
		irq_enable(DT_INST_IRQN(inst)); \
DT_INST_FOREACH_STATUS_OKAY(CYCLONEV_ETH_INIT)
